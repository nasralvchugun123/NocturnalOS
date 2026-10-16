set(BOOTLOADER_SOURCES
    UEFI/uefi.c
    UEFI/boot.c
    PE_loader/PE_loader.c
)

add_executable(bootloader ${BOOTLOADER_SOURCES})

target_include_directories(bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OS_SDK_INC}
)

target_compile_options(bootloader PRIVATE
    -g
    ${OS_FREESTANDING_FLAGS}
)

target_link_options(bootloader PRIVATE
    -nostdlib
    -nodefaultlibs
    -Wl,--entry=efi_main
    -Wl,--subsystem,10
)

set_target_properties(bootloader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OS_BOOT_DIR}"
    OUTPUT_NAME "BOOTX64"
    SUFFIX ".EFI"
)