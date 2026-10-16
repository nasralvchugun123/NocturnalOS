add_library(sdk_rt STATIC
    lib/rt/mem.c
    lib/rt/string.c
    lib/rt/unicode.c
    lib/rt/format.c
)
target_include_directories(sdk_rt PUBLIC
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/sdk/inc
    ${CMAKE_SOURCE_DIR}/sdk/lib
    ${CMAKE_SOURCE_DIR}/sdk/lib/rt
    ${CMAKE_SOURCE_DIR}/sdk/lib/crt
    ${CMAKE_SOURCE_DIR}/sdk/lib/ntdll
    ${CMAKE_SOURCE_DIR}/sdk/RTL      
)
target_compile_options(sdk_rt PRIVATE
    -ffreestanding -fno-stack-protector -fno-builtin -mno-red-zone
)

add_library(sdk_syscall STATIC inc/syscall/syscall.s)
target_include_directories(sdk_syscall PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library(sdk_ntdll STATIC
    lib/ntdll/memory.c
    lib/ntdll/file.c
    lib/ntdll/proc.c
    lib/ntdll/sync.c
    lib/ntdll/object.c
    lib/ntdll/con.c
    lib/ntdll/dll.c
)

target_link_libraries(sdk_ntdll PUBLIC sdk_syscall sdk_rt)
target_include_directories(sdk_ntdll PUBLIC
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/sdk/inc
    ${CMAKE_SOURCE_DIR}/sdk/lib
    ${CMAKE_SOURCE_DIR}/sdk/lib/rt
    ${CMAKE_SOURCE_DIR}/sdk/lib/crt
    ${CMAKE_SOURCE_DIR}/sdk/lib/ntdll
    ${CMAKE_SOURCE_DIR}/sdk/RTL
)

add_library(sdk_crt STATIC
    lib/crt/start.c
    lib/crt/init.c
    lib/crt/dllmain.c
    lib/crt/errno.c
    lib/crt/libc/stdlib/malloc.c
    lib/crt/libc/stdlib/exit.c
    lib/crt/libc/stdlib/abort.c
    lib/crt/libc/stdlib/atoi.c
    lib/crt/libc/stdio/printf.c
    lib/crt/libc/stdio/puts.c
    lib/crt/libc/stdio/putchar.c
)
target_link_libraries(sdk_crt PUBLIC sdk_ntdll sdk_rt)
target_include_directories(sdk_crt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/crt/libc/standart   
)