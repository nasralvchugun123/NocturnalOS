add_usr_executable(testload
    SOURCES testload.c
    OUTPUT_DIR "${OS_SYSROOT}/Shell/Tests"
)

target_link_libraries(testload PRIVATE ntdll_user)
add_dependencies(testload ntdll_user)