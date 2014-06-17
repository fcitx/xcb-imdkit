cmake_minimum_required(VERSION 2.8.12)
project(xcb-imdkit)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(CheckCCompilerFlag)
include(CheckFunctionExists)

check_c_compiler_flag("-std=c99" C99_SUPPORTED)
if (NOT C99_SUPPORTED)
    message(FATAL_ERROR "C99 is required to compile Fcitx.")
endif()

add_definitions(-D_GNU_SOURCE)
set(CMAKE_C_FLAGS "-fPIC -std=c99 -Wall -Wextra -Wno-unused-parameter ${CMAKE_C_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

find_package(XCB REQUIRED)

enable_testing()

include_directories(${XCB_INCLUDE_DIRS})
include_directories(src)

add_subdirectory(src)

add_subdirectory(test)
