cmake_minimum_required(VERSION 3.20)

set(OS_SDK_NTDLL_DIR "${CMAKE_SOURCE_DIR}/sdk/lib/ntdll")

set(NTDLL_USER_SOURCES
    NTdll.c
    ldr.c
    ldr_pe.c
    ldr_imports.c

    ${OS_SDK_NTDLL_DIR}/con.c
    ${OS_SDK_NTDLL_DIR}/dll.c
    ${OS_SDK_NTDLL_DIR}/file.c
    ${OS_SDK_NTDLL_DIR}/memory.c
    ${OS_SDK_NTDLL_DIR}/object.c
    ${OS_SDK_NTDLL_DIR}/proc.c
    ${OS_SDK_NTDLL_DIR}/sync.c

    ${CMAKE_SOURCE_DIR}/sdk/inc/syscall/syscall.s
)

add_library(ntdll_user SHARED ${NTDLL_USER_SOURCES})

target_include_directories(ntdll_user PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OS_SDK_NTDLL_DIR}
    ${OS_SDK_DIR}
    ${OS_SDK_DIR}/inc
    ${OS_SDK_DIR}/RTL
)

target_compile_options(ntdll_user PRIVATE
    ${OS_FREESTANDING_FLAGS}
    -fno-jump-tables
)

target_link_options(ntdll_user PRIVATE
    -nostdlib
    -nodefaultlibs
    -Wl,--entry=LdrpInitializeThunk
    -Wl,--subsystem,windows
    -Wl,--image-base=0x7FF000000000
    -Wl,--file-alignment,0x200
    -Wl,--section-alignment,0x1000
    -Wl,--export-all-symbols
)

set_target_properties(ntdll_user PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    OUTPUT_NAME "ntdll"
    RUNTIME_OUTPUT_DIRECTORY "${OS_SYSROOT}/Core/Sys64"
)

register_usr_binary(ntdll_user "${OS_SYSROOT}/Core/Sys64")
