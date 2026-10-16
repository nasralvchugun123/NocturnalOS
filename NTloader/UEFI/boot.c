
#include "uefi.h"
#include "../PE_loader/PE_loader.h"
#include "boot_info.h"

EFI_SYSTEM_TABLE *gST = NULL;
static EFI_HANDLE gImageHandle = NULL;

typedef void (EFIAPI *kentry_t)(BOOT_INFO *);

static BOOT_INFO g_boot_info;

static void ascii_to_utf16(const char *src, uint16_t *dst, size_t max)
{
    size_t i = 0;
    while (src[i] && i + 1 < max) {
        dst[i] = (uint16_t)(uint8_t)src[i];
        i++;
    }
    dst[i] = 0;
}

void efi_print(const char *s)
{
    if (!gST || !gST->ConOut || !gST->ConOut->OutputString) return;
    uint16_t buf[256];
    size_t i = 0;
    while (*s && i < 254) {
        uint16_t w = (uint16_t)(uint8_t)*s++;
        if (w == '\n') { buf[i++] = '\r'; buf[i++] = '\n'; }
        else            buf[i++] = w;
    }
    buf[i] = 0;
    gST->ConOut->OutputString(gST->ConOut, buf);
}

static void efi_print_hex(uint64_t v)
{
    char hex[19];
    hex[0] = '0'; hex[1] = 'x'; hex[18] = 0;
    for (int i = 17; i >= 2; i--) {
        int d = (int)(v & 0xF);
        hex[i] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v >>= 4;
    }
    efi_print(hex);
}

static void efi_print_dec(uint64_t v)
{
    char buf[24];
    int i = 23;
    buf[i] = 0;
    if (v == 0) buf[--i] = '0';
    while (v) { buf[--i] = (char)('0' + (v % 10)); v /= 10; }
    efi_print(&buf[i]);
}

static void *memset_local(void *dst, int c, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

static EFI_STATUS open_protocol(EFI_HANDLE handle, EFI_GUID *guid, void **out)
{
    EFI_STATUS st;
    if (gST->BootServices->OpenProtocol) {
        st = gST->BootServices->OpenProtocol(
                handle, guid, out,
                gImageHandle, NULL,
                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (st == EFI_SUCCESS) return EFI_SUCCESS;

        st = gST->BootServices->OpenProtocol(
                handle, guid, out,
                gImageHandle, NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (st == EFI_SUCCESS) return EFI_SUCCESS;
    }
    if (gST->BootServices->HandleProtocol) {
        st = gST->BootServices->HandleProtocol(handle, guid, out);
        if (st == EFI_SUCCESS) return EFI_SUCCESS;
    }
    return st;
}

static EFI_STATUS read_file(EFI_HANDLE image_handle,
                            const char *path_ascii,
                            void **out_buf, size_t *out_size,
                            const char **stage)
{
    (void)image_handle;
    EFI_STATUS st;
    EFI_GUID sfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID fi_guid  = EFI_FILE_INFO_GUID;

    *stage = "locate-all";
    EFI_HANDLE *handles = NULL;
    UINTN total = 0;
    st = gST->BootServices->LocateHandleBuffer(0, NULL, NULL, &total, &handles);
    if (st != EFI_SUCCESS || !handles || total == 0) {
        *stage = "no-handles";
        return EFI_NOT_FOUND;
    }

    uint16_t wpath[260];
    ascii_to_utf16(path_ascii, wpath, 260);

    *stage = "scan-all-sfs";
    EFI_STATUS last_err = EFI_NOT_FOUND;

    for (UINTN i = 0; i < total; i++) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs = NULL;
        EFI_STATUS hs = gST->BootServices->HandleProtocol(
                            handles[i], &sfs_guid, (void **)&sfs);
        if (hs != EFI_SUCCESS || !sfs) continue;

        EFI_FILE_PROTOCOL *root = NULL;
        if (sfs->OpenVolume(sfs, &root) != EFI_SUCCESS) continue;

        EFI_FILE_PROTOCOL *file = NULL;
        if (root->Open(root, &file, wpath, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS) {
            root->Close(root);
            continue;
        }

        uint8_t info_buf[1024];
        uint64_t info_size = sizeof(info_buf);
        if (file->GetInfo(file, &fi_guid, &info_size, info_buf) != EFI_SUCCESS) {
            file->Close(file); root->Close(root); continue;
        }
        EFI_FILE_INFO *fi = (EFI_FILE_INFO *)info_buf;
        uint64_t file_size = fi->FileSize;
        if (file_size == 0) {
            file->Close(file); root->Close(root); continue;
        }

        void *buf = NULL;
        if (gST->BootServices->AllocatePool(EfiLoaderData,
                                            (UINTN)file_size, &buf) != EFI_SUCCESS) {
            file->Close(file); root->Close(root); continue;
        }

        uint64_t read_size = file_size;
        EFI_STATUS rs = file->Read(file, &read_size, buf);
        if (rs != EFI_SUCCESS || read_size != file_size) {
            gST->BootServices->FreePool(buf);
            file->Close(file); root->Close(root);
            last_err = rs;
            continue;
        }

        file->Close(file);
        root->Close(root);
        gST->BootServices->FreePool(handles);

        *out_buf  = buf;
        *out_size = (size_t)file_size;
        *stage    = "ok";
        return EFI_SUCCESS;
    }

    gST->BootServices->FreePool(handles);
    *stage = "not-found-on-any-volume";
    return last_err;
}

static EFI_STATUS read_kernel(EFI_HANDLE ImageHandle, void **buf, size_t *sz)
{
    const char *candidates[] = {
        "\\NocturnalOS\\system\\core\\ntos\\ntoskrnl.exe",
        "\\kernel.efi",
        0
    };  

    EFI_STATUS st = EFI_LOAD_ERROR;
    for (int i = 0; candidates[i]; i++) {
        const char *stage = "?";
        st = read_file(ImageHandle, candidates[i], buf, sz, &stage);

        efi_print("  [");
        efi_print(st == EFI_SUCCESS ? "OK  " : "FAIL");
        efi_print("] ");
        efi_print(candidates[i]);
        efi_print("  stage=");
        efi_print(stage);
        efi_print("  status=");
        efi_print_hex((uint64_t)st);
        efi_print("\n");

        if (st == EFI_SUCCESS) return EFI_SUCCESS;
    }
    return st;
}

static void *kernel_alloc(size_t size)
{
    void *p = NULL;
    EFI_STATUS st = gST->BootServices->AllocatePool(EfiLoaderCode, (UINTN)size, &p);
    if (st != EFI_SUCCESS) {
        efi_print("  kernel_alloc FAILED size=");
        efi_print_dec(size);
        efi_print(" st=");
        efi_print_hex((uint64_t)st);
        efi_print("\n");
        return NULL;
    }
    return p;
}

static EFI_STATUS exit_boot_services(void)
{
    UINTN map_size = 0, map_key = 0, desc_size = 0;
    uint32_t desc_ver = 0;
    EFI_MEMORY_DESCRIPTOR *map = NULL;

    EFI_STATUS st = gST->BootServices->GetMemoryMap(&map_size, NULL, &map_key,
                                                    &desc_size, &desc_ver);
    map_size += 8 * desc_size;

    st = gST->BootServices->AllocatePool(EfiLoaderData, map_size, (void **)&map);
    if (st != EFI_SUCCESS) return st;

    for (int tries = 0; tries < 8; tries++) {
        st = gST->BootServices->GetMemoryMap(&map_size, map, &map_key,
                                             &desc_size, &desc_ver);
        if (st != EFI_SUCCESS) continue;

        st = gST->BootServices->ExitBootServices(gImageHandle, map_key);
        if (st == EFI_SUCCESS) return EFI_SUCCESS;
    }
    return st;
}

static EFI_GUID gAcpi20Guid = {
    0x8868E871, 0xE4F1, 0x11D3,
    {0xBC, 0x22, 0x00, 0x80, 0xC7, 0x3C, 0x88, 0x81}
};
static EFI_GUID gAcpi10Guid = {
    0xEB9D2D30, 0x2D88, 0x11D3,
    {0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}
};

static int guid_eq(const EFI_GUID *a, const EFI_GUID *b)
{
    if (a->Data1 != b->Data1) return 0;
    if (a->Data2 != b->Data2) return 0;
    if (a->Data3 != b->Data3) return 0;
    for (int i = 0; i < 8; i++) if (a->Data4[i] != b->Data4[i]) return 0;
    return 1;
}

static void fill_acpi_rsdp(BOOT_INFO *bi)
{
    if (!gST || !gST->ConfigurationTable || gST->NumberOfTableEntries == 0) {
        efi_print("  ACPI: no config table\n");
        return;
    }

    EFI_CONFIGURATION_TABLE *ct = gST->ConfigurationTable;
    for (UINTN i = 0; i < gST->NumberOfTableEntries; i++) {
        if (guid_eq(&ct[i].VendorGuid, &gAcpi20Guid) ||
            guid_eq(&ct[i].VendorGuid, &gAcpi10Guid)) {
            bi->AcpiRsdp = (uint64_t)(uintptr_t)ct[i].VendorTable;
            efi_print("  ACPI RSDP = ");
            efi_print_hex(bi->AcpiRsdp);
            efi_print("\n");
            return;
        }
    }
    efi_print("  ACPI RSDP: not found in config table\n");
}

static void fill_framebuffer_info(BOOT_INFO *bi)
{
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

    EFI_STATUS st = gST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&gop);
    if (st != EFI_SUCCESS || !gop || !gop->Mode || !gop->Mode->Info) {
        efi_print("  GOP not found\n");
        return;
    }

    uint32_t max_mode = gop->Mode->MaxMode;
    uint32_t best_mode = gop->Mode->Mode;   
    uint32_t best_w = gop->Mode->Info->HorizontalResolution;
    uint32_t best_h = gop->Mode->Info->VerticalResolution;

    efi_print("  scanning GOP modes (total=");
    efi_print_dec(max_mode);
    efi_print(")...\n");

    for (uint32_t m = 0; m < max_mode; m++) {
        UINTN info_size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;
        EFI_STATUS qs = gop->QueryMode(gop, m, &info_size, &info);
        if (qs != EFI_SUCCESS || !info) continue;

        uint32_t w = info->HorizontalResolution;
        uint32_t h = info->VerticalResolution;

        if (w == 1920 && h == 1080) {
            best_mode = m;
            best_w = w;
            best_h = h;
            efi_print("    found 1920x1080 at mode ");
            efi_print_dec(m);
            efi_print("\n");
            break;
        }
    }

    if (best_mode != gop->Mode->Mode) {
        EFI_STATUS ss = gop->SetMode(gop, best_mode);
        if (ss != EFI_SUCCESS) {
            efi_print("  SetMode failed, staying on current\n");
            best_mode = gop->Mode->Mode;
        }
    }
    
    efi_print("  GOP final: ");
    efi_print_dec(best_w);
    efi_print("x");
    efi_print_dec(best_h);
    efi_print("\n");

    bi->FramebufferBase              = gop->Mode->FrameBufferBase;
    bi->FramebufferSize              = gop->Mode->FrameBufferSize;
    bi->FramebufferWidth             = gop->Mode->Info->HorizontalResolution;
    bi->FramebufferHeight            = gop->Mode->Info->VerticalResolution;
    bi->FramebufferPixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;
    bi->FramebufferFormat            = (uint32_t)gop->Mode->Info->PixelFormat;

    efi_print("  GOP: ");
    efi_print_dec(bi->FramebufferWidth);
    efi_print("x");
    efi_print_dec(bi->FramebufferHeight);
    efi_print(" stride=");
    efi_print_dec(bi->FramebufferPixelsPerScanLine);
    efi_print(" format=");
    efi_print_dec(bi->FramebufferFormat);
    efi_print(" base=");
    efi_print_hex(bi->FramebufferBase);
    efi_print("\n");

}

static EFI_STATUS fill_memory_map_info(BOOT_INFO *bi)
{
    UINTN map_size = 0, map_key = 0, desc_size = 0;
    uint32_t desc_ver = 0;
    void *buf = NULL;
    EFI_STATUS st;

    st = gST->BootServices->GetMemoryMap(&map_size, NULL, &map_key,
                                         &desc_size, &desc_ver);
    if (desc_size == 0) desc_size = sizeof(EFI_MEMORY_DESCRIPTOR);
    if (map_size < desc_size * 32) map_size = desc_size * 32;

    for (int attempt = 0; attempt < 8; attempt++) {
        map_size += desc_size * 8;  

        st = gST->BootServices->AllocatePool(EfiLoaderData, map_size, &buf);
        if (st != EFI_SUCCESS) return st;

        st = gST->BootServices->GetMemoryMap(&map_size,
                                             (EFI_MEMORY_DESCRIPTOR *)buf,
                                             &map_key, &desc_size, &desc_ver);
        if (st == EFI_SUCCESS) {
            bi->MemoryMap                  = (uint64_t)(uintptr_t)buf;
            bi->MemoryMapSize              = (uint64_t)map_size;
            bi->MemoryMapDescriptorSize    = (uint64_t)desc_size;
            bi->MemoryMapDescriptorVersion = desc_ver;
            bi->AcpiRsdp                   = 0;
            return EFI_SUCCESS;
        }

        gST->BootServices->FreePool(buf);
        buf = NULL;

        if (st != EFI_BUFFER_TOO_SMALL) return st;
    }
    return st;
}


EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    gST = SystemTable;
    gImageHandle = ImageHandle;

    efi_print("                                                      < NTloader is started! >                                                    \n");

    efi_print("\n");
    efi_print("<BootLoader Trace>\n");
    efi_print("################################################################################################################################# \n");

    efi_print("Reading kernel:\n");
    void *file_buf = NULL;
    size_t file_size = 0;
    EFI_STATUS st = read_kernel(ImageHandle, &file_buf, &file_size);
    if (st != EFI_SUCCESS) {
        efi_print("FATAL: cannot read kernel.\n");
        return st;
    }
    efi_print("Kernel file size = ");
    efi_print_dec((uint64_t)file_size);
    efi_print("\n");

    efi_print("Loading PE image:\n");
    PE_LOADED_IMAGE img;
    int pe_err = PE_LoadImage(file_buf, file_size, kernel_alloc, &img);
    if (pe_err != 0) {
        efi_print("  PE_LoadImage FAILED, code=");
        efi_print_dec((uint64_t)(int64_t)pe_err);
        efi_print("\n");
        return EFI_LOAD_ERROR;
    }
    efi_print("  ImageBase   = "); efi_print_hex((uint64_t)(uintptr_t)img.ImageBase); efi_print("\n");
    efi_print("  EntryPoint  = "); efi_print_hex(img.EntryPoint); efi_print("\n");
    efi_print("  SizeOfImage = "); efi_print_dec(img.SizeOfImage); efi_print("\n");

    gST->BootServices->FreePool(file_buf);

    efi_print("Building BOOT_INFO:\n");
    memset_local(&g_boot_info, 0, sizeof(g_boot_info));
    g_boot_info.Magic          = BOOT_INFO_MAGIC;
    g_boot_info.Version        = BOOT_INFO_VERSION;
    g_boot_info.Size           = (uint32_t)sizeof(BOOT_INFO);
    g_boot_info.KernelBase     = (uint64_t)(uintptr_t)img.ImageBase;
    g_boot_info.KernelSize     = img.SizeOfImage;
    g_boot_info.EntryPoint     = img.EntryPoint;
    g_boot_info.EfiSystemTable = (uint64_t)(uintptr_t)gST;

    fill_framebuffer_info(&g_boot_info);

    efi_print("Copying memory map:\n");
    st = fill_memory_map_info(&g_boot_info);
    if (st != EFI_SUCCESS) {
        efi_print("  GetMemoryMap FAILED, st=");
        efi_print_hex((uint64_t)st);
        efi_print("\n");
        return st;
    }
    efi_print("  size=");
    efi_print_dec(g_boot_info.MemoryMapSize);
    efi_print(" desc_size=");
    efi_print_dec(g_boot_info.MemoryMapDescriptorSize);
    efi_print(" map_at=");
    efi_print_hex(g_boot_info.MemoryMap);
    efi_print("\n");

    efi_print("Finding ACPI RSDP:\n");
    fill_acpi_rsdp(&g_boot_info);

    efi_print("Allocating kernel stack:\n");
    void *kstack = NULL;
    st = gST->BootServices->AllocatePool(EfiLoaderData, 64 * 1024, &kstack);
    if (st != EFI_SUCCESS || !kstack) {
        efi_print("  AllocatePool FAILED, status=");
        efi_print_hex((uint64_t)st);
        efi_print("\n");
        return st;
    }
    uint64_t stack_top = ((uint64_t)(uintptr_t)kstack + 64 * 1024) & ~0xFULL;
    g_boot_info.StackTop = stack_top;
    efi_print("  StackTop = "); efi_print_hex(stack_top); efi_print("\n");

    efi_print("Exiting boot services...\n");
    efi_print("#####################################################################################################\n");
    st = exit_boot_services();
    if (st != EFI_SUCCESS) {
        efi_print("  ExitBootServices FAILED, status=");
        efi_print_hex((uint64_t)st);
        efi_print("\n");
        return st;
    }
    
    BOOT_INFO *bi     = &g_boot_info;
    kentry_t   kmain  = (kentry_t)(uintptr_t)img.EntryPoint;
    uint64_t   new_sp = stack_top;

    __asm__ volatile (
        "mov %0, %%rcx\n"
        "mov %1, %%rsp\n"
        "mov %1, %%rbp\n"
        "jmp *%2\n"
        :
        : "r"(bi), "r"(new_sp), "r"(kmain)
        : "rcx", "memory"
    );

    for (;;) __asm__ volatile ("hlt");
    return EFI_SUCCESS;

}