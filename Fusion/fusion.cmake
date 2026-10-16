add_usr_executable(fusion
    SOURCES
        ser.c
        intinit/commandlist.c
        intinit/init/entry.c
        intinit/builtin/commands/help.c
        intinit/builtin/commands/echo.c
        intinit/builtin/commands/ver.c
        intinit/builtin/commands/exit.c
        intinit/builtin/commands/ls.c
        intinit/builtin/commands/cd.c
        intinit/builtin/commands/cat.c
        intinit/builtin/commands/find.c
        intinit/builtin/commands/dlltest.c

    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_DIR "${OS_SYSROOT}/Shell/Fusion"
)

target_link_options(fusion PRIVATE -Wl,--image-base=0x8002000000)
