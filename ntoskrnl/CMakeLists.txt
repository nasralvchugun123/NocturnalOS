set(NTOSKRNL_SOURCES
    #Ke
    ke/fatal.c
    ke/exept.c
    ke/time.c
    ke/clock.c
    ke/fatal.c
    ke/time.c
    ke/wait.c
    ke/kbd.c
    ke/kbdmap.c
    ke/kbdinput.c
    ke/shell.c
    ke/ke_init.c
    ke/spinlock.c
    ke/kcpu.c

    ke/x64/gdt.c
    ke/x64/switch.S
    ke/x64/idt.c
    ke/x64/isr.S
    ke/x64/syscall_entry.S
    ke/x64/except.c
    ke/x64/usrmod.S
    ke/x64/syscall.c
  
    #sheduler
    sched/al/RR/mp/sched.c

    #debug
    dbg/print/kdprint.c

    #display
    ds/fb/fb.c

    # ki
    ki/Ki_init.c
    ki/ki_ap.c

    # Mm
    mm/memory/mm_init.c
    mm/memory/pmm.c
    mm/memory/vmm.c

    # Ps
    ps/ps_init.c
    ps/thread.c

    # Ob
    ob/obcache.c
    ob/reference.c
    ob/handle.c
    ob/Init.c
    ob/wait.c
    ob/obname.c

    # Io
    io/io_init.c
    io/driver.c
    io/device.c
    io/irp.c
    io/dispatch.c

    # dll
    dll/imgsec.c
    dll/loader.c
    dll/dllsys.c

    # drivers

    #nvme
    ${OS_SOURCE_DIR}/drivers/storage/nvme/nvme.c
    # gpt/mbr
    ${OS_SOURCE_DIR}/drivers/fs/GPT/gpt.c
    #fs
    ${OS_SOURCE_DIR}/drivers/fs/fat/fat32/fat32.c
    #vfsdrrv
    ${OS_SOURCE_DIR}/drivers/fs/vfs/vfs.c
)

add_executable(ntoskrnl ${NTOSKRNL_SOURCES})

target_include_directories(ntoskrnl PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/sdk/inc
    ${CMAKE_SOURCE_DIR}/sdk/RTL

    ${CMAKE_SOURCE_DIR}/ntoskrnl
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ke
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ke/arch
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ke/x64
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ki
    ${CMAKE_SOURCE_DIR}/ntoskrnl/mm
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ob
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ps
    ${CMAKE_SOURCE_DIR}/ntoskrnl/io
    ${CMAKE_SOURCE_DIR}/ntoskrnl/fs
    ${CMAKE_SOURCE_DIR}/ntoskrnl/cc
    ${CMAKE_SOURCE_DIR}/ntoskrnl/dll
    ${CMAKE_SOURCE_DIR}/ntoskrnl/sched
    ${CMAKE_SOURCE_DIR}/ntoskrnl/dbg
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ds
    ${CMAKE_SOURCE_DIR}/ntoskrnl/cc
    
    # hal
    ${CMAKE_SOURCE_DIR}/hal
    ${CMAKE_SOURCE_DIR}/hal/halx64
    ${CMAKE_SOURCE_DIR}/hal/halx64/Apic
    ${CMAKE_SOURCE_DIR}/hal/halx64/Init
    ${CMAKE_SOURCE_DIR}/hal/halx64/PCI

    # drivers
    ${CMAKE_SOURCE_DIR}/drivers/storage/nvme
    ${CMAKE_SOURCE_DIR}/drivers/storage/virtio
    ${CMAKE_SOURCE_DIR}/drivers/fs/GPT
    ${CMAKE_SOURCE_DIR}/drivers/fs/fat/fat32
    ${CMAKE_SOURCE_DIR}/drivers/fs/vfs
)

target_compile_options(ntoskrnl PRIVATE
    ${OS_FREESTANDING_FLAGS}
    -mgeneral-regs-only
    -D_KERNEL
    -fno-builtin
    -fno-builtin-memcpy
    -fno-builtin-memmove
    -fno-builtin-memset
    -fno-builtin-memcmp
    -fno-jump-tables
)

target_link_options(ntoskrnl PRIVATE
    -nostdlib
    -nodefaultlibs
    -Wl,--entry=KiSystemStartup
    -Wl,--subsystem,native
    -Wl,--image-base,0x400000
    -Wl,--file-alignment,0x200
    -Wl,--section-alignment,0x1000
)

target_link_libraries(ntoskrnl PRIVATE hal sdk_rt)

set_target_properties(ntoskrnl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OS_SYSROOT}/Core/ntos"
)