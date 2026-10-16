cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchain-Linux.cmake"
        CACHE FILEPATH "Default toolchain" FORCE)
endif()

project(NocturnalOS VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_CREATE_WIN32_EXE     "")
set(CMAKE_C_CREATE_CONSOLE_EXE   "")
set(CMAKE_CXX_CREATE_WIN32_EXE   "")
set(CMAKE_CXX_CREATE_CONSOLE_EXE "")

set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_C_STANDARD_LIBRARIES   "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(OS_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
set(OS_BINARY_DIR "${CMAKE_BINARY_DIR}")
set(OS_SDK_DIR    "${OS_SOURCE_DIR}/sdk")
set(OS_SDK_INC    "${OS_SDK_DIR}/inc")
set(OS_ROOT       "${OS_BINARY_DIR}/NocturnalOS")
set(OS_SYSROOT    "${OS_ROOT}/System")
set(OS_BOOT_DIR   "${OS_BINARY_DIR}/boot")
set(OS_ISO_ROOT   "${OS_BINARY_DIR}/iso_root")
set(OS_ISO_IMAGE  "${OS_BINARY_DIR}/NocturnalOS.iso")
set(OS_ESP_DIR    "${OS_BINARY_DIR}/esp")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_compile_options(
    -m64 
    -Wall
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-error=implicit-function-declaration
    -Wno-error=implicit-int
    -Wno-error=incompatible-function-pointer-types
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion
    -Wno-error=return-mismatch
    -Wno-error=return-type
    -Wno-error=declaration-after-statement
    -Wno-error=strict-prototypes
    -Wno-error=deprecated-non-prototype
)

include_directories(${OS_SDK_INC})


include(cmake/NocturnalOS.cmake)
include(cmake/sysroot.cmake)

add_subdirectory(sdk)
add_subdirectory(hal)
add_subdirectory(NTloader)
add_subdirectory(ntoskrnl)
add_subdirectory(base)
add_subdirectory(Fusion)

include(cmake/iso.cmake)
