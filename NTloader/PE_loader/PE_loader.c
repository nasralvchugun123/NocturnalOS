
#include "PE_loader.h"

static void *memset_local(void *dst, int c, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

static void *memcpy_local(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

static int rva_in_image(uint32_t rva, uint32_t size, uint32_t image_size)
{
    return (uint64_t)rva + size <= image_size;
}

int PE_LoadImage(const void *FileBuffer, size_t FileSize,
                 void *(*alloc_cb)(size_t),
                 PE_LOADED_IMAGE *Out)
{
    if (!FileBuffer || !alloc_cb || !Out)
        return -1;

    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)FileBuffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return -2;
    if (dos->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) > FileSize)
        return -3;

    const uint8_t *nt = (const uint8_t *)FileBuffer + dos->e_lfanew;
    if (*(const uint32_t *)nt != IMAGE_NT_SIGNATURE)
        return -4;

    const IMAGE_FILE_HEADER *fh = (const IMAGE_FILE_HEADER *)(nt + 4);
    const IMAGE_OPTIONAL_HEADER64 *oh =
        (const IMAGE_OPTIONAL_HEADER64 *)(nt + 4 + sizeof(IMAGE_FILE_HEADER));

    if (oh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return -5;
    if (oh->SizeOfImage == 0 || oh->SizeOfImage > (64u * 1024u * 1024u))
        return -6;

    uint8_t *image = (uint8_t *)alloc_cb(oh->SizeOfImage);
    if (!image)
        return -7;
    memset_local(image, 0, oh->SizeOfImage);

    uint32_t headers_size = oh->SizeOfHeaders;
    if (headers_size > FileSize) headers_size = (uint32_t)FileSize;
    if (headers_size > oh->SizeOfImage) headers_size = oh->SizeOfImage;
    memcpy_local(image, FileBuffer, headers_size);

    const IMAGE_SECTION_HEADER *sec =
        (const IMAGE_SECTION_HEADER *)((const uint8_t *)oh + fh->SizeOfOptionalHeader);

    for (uint16_t i = 0; i < fh->NumberOfSections; i++) {
        if (sec[i].SizeOfRawData == 0)
            continue;
        if (sec[i].PointerToRawData + sec[i].SizeOfRawData > FileSize)
            return -8;
        if (sec[i].VirtualAddress + sec[i].SizeOfRawData > oh->SizeOfImage)
            return -9;

        memcpy_local(image + sec[i].VirtualAddress,
                     (const uint8_t *)FileBuffer + sec[i].PointerToRawData,
                     sec[i].SizeOfRawData);
    }

    uint64_t preferred = oh->ImageBase;
    uint64_t actual    = (uint64_t)(uintptr_t)image;

    if (preferred != actual && oh->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC) {
        IMAGE_DATA_DIRECTORY rel = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (rel.VirtualAddress && rel.Size) {
            if (!rva_in_image(rel.VirtualAddress, rel.Size, oh->SizeOfImage))
                return -10;

            int64_t delta = (int64_t)(actual - preferred);
            uint32_t off = 0;
            while (off + sizeof(IMAGE_BASE_RELOCATION) <= rel.Size) {
                IMAGE_BASE_RELOCATION *blk =
                    (IMAGE_BASE_RELOCATION *)(image + rel.VirtualAddress + off);
                if (blk->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION))
                    break;

                uint32_t n = (blk->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;
                uint16_t *entries = (uint16_t *)((uint8_t *)blk + sizeof(IMAGE_BASE_RELOCATION));

                for (uint32_t k = 0; k < n; k++) {
                    uint16_t type   = entries[k] >> 12;
                    uint16_t offset = entries[k] & 0x0FFF;
                    uint32_t patch_rva = blk->VirtualAddress + offset;

                    if (!rva_in_image(patch_rva, 8, oh->SizeOfImage))
                        continue;

                    uint8_t *patch_ptr = image + patch_rva;
                    switch (type) {
                        case IMAGE_REL_BASED_ABSOLUTE:
                            break;
                        case IMAGE_REL_BASED_DIR64:
                            *(uint64_t *)patch_ptr += (uint64_t)delta;
                            break;
                        case IMAGE_REL_BASED_HIGHLOW:
                            *(uint32_t *)patch_ptr += (uint32_t)delta;
                            break;
                        default:
                            return -11; // unknown rel type
                    }
                }

                off += blk->SizeOfBlock;
            }
        }
    }

    Out->ImageBase    = image;
    Out->EntryPoint   = actual + oh->AddressOfEntryPoint;
    Out->SizeOfImage  = oh->SizeOfImage;
    return 0;
}