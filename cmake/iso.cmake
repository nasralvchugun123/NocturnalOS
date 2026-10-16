
set(ESP_FILES
    "${OS_BOOT_DIR}/BOOTX64.EFI"                          "EFI/BOOT/BOOTX64.EFI"
    "${OS_SYSROOT}/Core/ntos/ntoskrnl.exe"                "NocturnalOS/System/Core/ntos/ntoskrnl.exe"
)

set(ESP_DEPS bootloader ntoskrnl)

get_property(_usr_reg GLOBAL PROPERTY OS_USER_BINARIES)

foreach(_entry IN LISTS _usr_reg)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _tgt)
    list(GET _parts 1 _dir)

    if (NOT _dir)
        message(WARNING "OS_USER_BINARIES: ${_tgt} без OUTPUT_DIR — пропуск")
        continue()
    endif()

    get_target_property(_outname ${_tgt} OUTPUT_NAME)
    if (NOT _outname OR _outname STREQUAL "_outname-NOTFOUND")
        set(_outname ${_tgt})
    endif()

    get_target_property(_suffix ${_tgt} SUFFIX)
    if (NOT _suffix OR _suffix STREQUAL "_suffix-NOTFOUND")
        get_target_property(_type ${_tgt} TYPE)
        if (_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
            set(_suffix ".dll")
        else()
            set(_suffix ".exe")
        endif()
    endif()

    string(FIND "${_outname}" "${_suffix}" _has_suffix)
    if (_has_suffix GREATER -1)
        set(_fname "${_outname}")
    else()
        set(_fname "${_outname}${_suffix}")
    endif()

    file(RELATIVE_PATH _rel "${OS_SYSROOT}" "${_dir}")
    set(_src "${_dir}/${_fname}")
    set(_dst "NocturnalOS/System/${_rel}/${_fname}")

    list(APPEND ESP_FILES "${_src}" "${_dst}")
    list(APPEND ESP_DEPS ${_tgt})

    message(STATUS "  [esp] ${_fname}  →  NocturnalOS/System/${_rel}/")
endforeach()

set(ESP_DIRS
    EFI/BOOT
    NocturnalOS/System/Core/ntos
    NocturnalOS/System/Core/Krbtdev
    NocturnalOS/System/Core/Sys64
    NocturnalOS/System/Drivers/Ess
    NocturnalOS/System/Drivers/Usr
    NocturnalOS/System/Drivers/Machine
    NocturnalOS/System/Drivers/Bus
    NocturnalOS/System/Drivers/Uenv
    NocturnalOS/System/Drivers/Storage
    NocturnalOS/System/Drivers/Filesys
    NocturnalOS/System/Shell/Fusion
    NocturnalOS/System/Shell/Subsys
    NocturnalOS/System/Shell/Sctlctr
    NocturnalOS/System/Shell/Themes
    NocturnalOS/System/Shell/Logon
    NocturnalOS/System/Shell/Tests
    NocturnalOS/System/X32ss
)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_CLEAN_ISO_ROOT
        cmd /c "if exist \"${OS_ISO_ROOT}\" (attrib -R -S -H \"${OS_ISO_ROOT}\\*\" /S /D 2>NUL & rd /S /Q \"${OS_ISO_ROOT}\")")
    set(_CLEAN_ESP_DIR
        cmd /c "if exist \"${OS_ESP_DIR}\" (attrib -R -S -H \"${OS_ESP_DIR}\\*\" /S /D 2>NUL & rd /S /Q \"${OS_ESP_DIR}\")")
else()
    set(_CLEAN_ISO_ROOT "${CMAKE_COMMAND}" -E rm -rf "${OS_ISO_ROOT}")
    set(_CLEAN_ESP_DIR  "${CMAKE_COMMAND}" -E rm -rf "${OS_ESP_DIR}")
endif()

set(_esp_cmds "")
list(APPEND _esp_cmds
    COMMAND ${_CLEAN_ESP_DIR}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OS_ESP_DIR}"
)
foreach(_d IN LISTS ESP_DIRS)
    list(APPEND _esp_cmds
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OS_ESP_DIR}/${_d}"
    )
endforeach()

list(LENGTH ESP_FILES _len)
if(_len GREATER 0)
    math(EXPR _max "${_len} - 1")
    foreach(_i RANGE 0 ${_max} 2)
        list(GET ESP_FILES ${_i} _src)
        math(EXPR _j "${_i} + 1")
        list(GET ESP_FILES ${_j} _dst)
        list(APPEND _esp_cmds
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${_src}" "${OS_ESP_DIR}/${_dst}"
        )
    endforeach()
endif()

add_custom_target(esp
    ${_esp_cmds}
    DEPENDS ${ESP_DEPS}
    VERBATIM
    COMMAND_EXPAND_LISTS
    COMMENT ">>> Собираю staging ESP: ${OS_ESP_DIR}"
)

find_program(XORRISO_EXEC NAMES xorriso
    HINTS
        "$ENV{ProgramFiles}/xorriso"
        "C:/msys64/mingw64/bin"
        "C:/msys64/usr/bin"
        "$ENV{USERPROFILE}/scoop/shims"
)
find_program(MCOPY_EXEC NAMES mcopy
    HINTS "C:/msys64/mingw64/bin" "C:/msys64/usr/bin" "$ENV{USERPROFILE}/scoop/shims"
)

if(NOT (XORRISO_EXEC AND MCOPY_EXEC))
    message(STATUS "NocturnalOS: xorriso/mtools не найдены → target 'iso' пропущен")
    return()
endif()

message(STATUS "NocturnalOS: xorriso+mtools найдены → target 'iso' доступен")

set(MTTOOLSRC_FILE "${OS_BINARY_DIR}/mtoolsrc")
file(WRITE "${MTTOOLSRC_FILE}"
    "mtools_skip_check=1\n"
    "MTOOLS_SKIP_CHECK=1\n"
)

set(MT_ENV
    "${CMAKE_COMMAND}" -E env
    "MTOOLSRC=${MTTOOLSRC_FILE}"
    "MTOOLS_SKIP_CHECK=1"
)

set(EFIBOOT_SIZE_MB 256)
set(EFIBOOT_IMG "${OS_ISO_ROOT}/efiboot.img")

add_custom_target(iso
    COMMAND ${_CLEAN_ISO_ROOT}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OS_ISO_ROOT}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${OS_ESP_DIR}" "${OS_ISO_ROOT}"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${EFIBOOT_IMG}"

    COMMAND dd if=/dev/zero "of=${EFIBOOT_IMG}" bs=1M count=${EFIBOOT_SIZE_MB} status=none
    COMMAND mkfs.vfat -F 32 -n EFIBOOT "${EFIBOOT_IMG}"

    COMMAND ${MT_ENV} "${MCOPY_EXEC}" -o -s -i "${EFIBOOT_IMG}" "${OS_ISO_ROOT}/EFI" ::/
    COMMAND ${MT_ENV} "${MCOPY_EXEC}" -o -s -i "${EFIBOOT_IMG}" "${OS_ISO_ROOT}/NocturnalOS" ::/

    COMMAND "${CMAKE_COMMAND}" -E rm -f "${OS_ISO_IMAGE}"
    COMMAND "${XORRISO_EXEC}"
            -as mkisofs
            -V NocturnalOS
            -iso-level 3
            -full-iso9660-filenames
            -R
            -J
            -joliet-long
            -e efiboot.img
            -no-emul-boot
            -isohybrid-gpt-basdat
            -o "${OS_ISO_IMAGE}"
            "${OS_ISO_ROOT}"

    DEPENDS esp disk
    VERBATIM
    COMMAND_EXPAND_LISTS
    WORKING_DIRECTORY "${OS_BINARY_DIR}"
    COMMENT ">>> Building UEFI ISO: ${OS_ISO_IMAGE}"
)

find_program(PARTED_EXEC NAMES parted REQUIRED)

set(OS_DISK_IMAGE "${OS_BINARY_DIR}/NocturnalOS.disk.img")
set(DISK_SIZE_MB  64)
set(PART_START_MB 1)
set(PART_END_MB   63)

math(EXPR PART_START_SECTOR "${PART_START_MB} * 2048")
math(EXPR PART_SIZE_MB      "${PART_END_MB} - ${PART_START_MB}")

set(TMP_FAT_IMG "${OS_BINARY_DIR}/disk_fat.tmp")

add_custom_target(disk
    COMMAND dd if=/dev/zero "of=${OS_DISK_IMAGE}" bs=1M count=${DISK_SIZE_MB} status=none

    COMMAND "${PARTED_EXEC}" --script "${OS_DISK_IMAGE}" mklabel gpt
    COMMAND "${PARTED_EXEC}" --script "${OS_DISK_IMAGE}"
            mkpart primary fat32 ${PART_START_MB}MiB ${PART_END_MB}MiB
    COMMAND "${PARTED_EXEC}" --script "${OS_DISK_IMAGE}" set 1 msftdata on

    COMMAND dd if=/dev/zero "of=${TMP_FAT_IMG}" bs=1M count=${PART_SIZE_MB} status=none
    COMMAND mkfs.fat -F 32 -n NOCTURNAL "${TMP_FAT_IMG}"

    COMMAND ${MT_ENV} "${MCOPY_EXEC}" -o -s -i "${TMP_FAT_IMG}" "${OS_ESP_DIR}/NocturnalOS" ::/
    COMMAND ${MT_ENV} "${MCOPY_EXEC}" -o -s -i "${TMP_FAT_IMG}" "${OS_ESP_DIR}/EFI"         ::/

    COMMAND dd "if=${TMP_FAT_IMG}" "of=${OS_DISK_IMAGE}"
            bs=512 seek=${PART_START_SECTOR} conv=notrunc status=none

    COMMAND "${CMAKE_COMMAND}" -E rm -f "${TMP_FAT_IMG}"

    DEPENDS esp
    VERBATIM
    COMMENT ">>> Building NVMe disk image: ${OS_DISK_IMAGE}"
)
