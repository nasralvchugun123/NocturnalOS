
add_usr_executable(csrss
    SOURCES   csrss/csr.c
    OUTPUT_DIR "${OS_SYSROOT}/Shell/Subsys"
)
target_link_options(csrss PRIVATE -Wl,--image-base=0x8001000000)


add_usr_executable(services
    SOURCES   services/srv.c
    OUTPUT_DIR "${OS_SYSROOT}/Shell/Subsys"
)
target_link_options(services PRIVATE -Wl,--image-base=0x8000000000)

add_subdirectory(NTDLL)
add_dependencies(ntoskrnl ntdll_user)  
