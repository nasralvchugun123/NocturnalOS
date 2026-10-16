set(OS_FREESTANDING_FLAGS
    -ffreestanding
    -fno-builtin
    -fno-stack-protector
    -mno-red-zone
    -mno-stack-arg-probe
    -fno-asynchronous-unwind-tables
)

define_property(GLOBAL PROPERTY OS_USER_BINARIES
    BRIEF_DOCS "List of 'target|output_dir' user binaries to stage into ESP")

define_property(GLOBAL PROPERTY OS_NEXT_IMAGE_BASE
    BRIEF_DOCS "Next auto-assigned image base for user executables")

function(register_usr_binary target_name output_dir)
    if (NOT target_name OR NOT output_dir)
        return()
    endif()
    get_property(_reg GLOBAL PROPERTY OS_USER_BINARIES)
    list(APPEND _reg "${target_name}|${output_dir}")
    set_property(GLOBAL PROPERTY OS_USER_BINARIES "${_reg}")
endfunction()

function(add_krnl_component NAME)
    cmake_parse_arguments(KC "" "" "SOURCES;INCLUDES" ${ARGN})
    add_library(${NAME} STATIC ${KC_SOURCES})
    target_include_directories(${NAME} PUBLIC ${KC_INCLUDES})
    target_compile_options(${NAME} PRIVATE
        ${OS_FREESTANDING_FLAGS}
        -mgeneral-regs-only
    )
endfunction()

function(add_usr_executable name)
    cmake_parse_arguments(
        AUE                          # pref
        ""                           # options (boolean)
        "OUTPUT_DIR;IMAGE_BASE"      # one-value kw
        "SOURCES;INCLUDES"           # multi-value kw
        ${ARGN}
    )

    if (NOT AUE_SOURCES)
        message(FATAL_ERROR "add_usr_executable(${name}): no SOURCES")
    endif()

    add_executable(${name} ${AUE_SOURCES})

    target_link_libraries(${name} PRIVATE sdk_crt)

    target_include_directories(${name} PRIVATE
        ${OS_SDK_DIR}/inc
        ${OS_SDK_DIR}/RTL
        ${CMAKE_SOURCE_DIR}/sdk/lib/crt/libc/standart
        ${AUE_INCLUDES}
    )

    target_compile_options(${name} PRIVATE
        ${OS_FREESTANDING_FLAGS}
        -mno-red-zone
        -fno-stack-protector
        -fno-builtin
        -fno-jump-tables
    )

    if (NOT AUE_IMAGE_BASE)
        get_property(_next GLOBAL PROPERTY OS_NEXT_IMAGE_BASE)
        if (NOT _next)
            set(_next 549755813888)   # 0x8000000000
        endif()
        set(AUE_IMAGE_BASE ${_next})
        math(EXPR _next "${_next} + 268435456")   # +0x10000000 (256 MiB)
        set_property(GLOBAL PROPERTY OS_NEXT_IMAGE_BASE ${_next})
        message(STATUS "  [img-base] ${name} = ${AUE_IMAGE_BASE}")
    endif()

    target_link_options(${name} PRIVATE
        -nostdlib
        -nodefaultlibs
        -Wl,--entry=_start
        -Wl,--subsystem,console
        -Wl,--image-base=${AUE_IMAGE_BASE}
        -Wl,--file-alignment,0x200
        -Wl,--section-alignment,0x1000
    )

    if (AUE_OUTPUT_DIR)
        set_target_properties(${name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${AUE_OUTPUT_DIR}"
        )
    endif()

    register_usr_binary(${name} "${AUE_OUTPUT_DIR}")
endfunction()