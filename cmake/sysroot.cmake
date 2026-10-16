set(OS_SYSROOT_TREE
    Core/Krbtdev
    Core/ntos
    Core/Sys64
    Drivers/Ess
    Drivers/Usr
    Drivers/Machine
    Drivers/Bus
    Drivers/Uenv
    Drivers/Storage
    Drivers/Filesys
    Shell/Fusion
    Shell/Subsys
    Shell/Sctlctr
    Shell/Themes
    Shell/Logon
    Shell/Tests
    X32ss
)

file(MAKE_DIRECTORY "${OS_ROOT}")
file(MAKE_DIRECTORY "${OS_SYSROOT}")
file(MAKE_DIRECTORY "${OS_BOOT_DIR}")

foreach(_dir IN LISTS OS_SYSROOT_TREE)
    file(MAKE_DIRECTORY "${OS_SYSROOT}/${_dir}")
endforeach()