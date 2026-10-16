add_library(testdll_user SHARED testdll.c testdll.def)

target_include_directories(testdll_user PRIVATE
    ${OS_SDK_DIR}
    ${OS_SDK_DIR}/inc
    ${OS_SDK_DIR}/RTL
)

target_compile_options(testdll_user PRIVATE
    ${OS_FREESTANDING_FLAGS}
    -fno-jump-tables
)

target_link_options(testdll_user PRIVATE
    -nostdlib
    -nodefaultlibs
    -Wl,--entry=DllMain
    -Wl,--subsystem,windows
    -Wl,--image-base=0x7FF100000000
    -Wl,--file-alignment,0x200
    -Wl,--section-alignment,0x1000
    -Wl,--export-all-symbols
)

set_target_properties(testdll_user PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    OUTPUT_NAME "testdll"
    RUNTIME_OUTPUT_DIRECTORY "${OS_SYSROOT}/Shell/Tests"
)

register_usr_binary(testdll_user "${OS_SYSROOT}/Shell/Tests")