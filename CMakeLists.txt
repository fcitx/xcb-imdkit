cmake_minimum_required(VERSION 2.8.12)
project(xcb-imdkit)

set(XCB_IMDKIT_VERSION 0.1.0)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(CheckFunctionExists)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(uninstall)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 11)

include(GNUInstallDirs)

# directory needed by bsd
if(NOT CMAKE_INSTALL_LIBDATADIR)
  set(CMAKE_INSTALL_LIBDATADIR "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "libdata root (LIBDIR)")
  set(CMAKE_INSTALL_LIBDATADIR "${CMAKE_INSTALL_LIBDIR}")
  mark_as_advanced(CMAKE_INSTALL_LIBDATADIR)
endif()

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/XCBImdkit")

add_definitions(-D_GNU_SOURCE)
set(CMAKE_C_FLAGS "-fPIC -std=c11 -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed -Wl,--no-undefined ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

find_package(XCB REQUIRED COMPONENTS XCB UTIL KEYSYMS)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)

install (
   EXPORT XCBImdkitLibraryTargets
   DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
   FILE XCBImdkitLibraryTargets.cmake
   NAMESPACE XCBImdkit::
   )

ecm_setup_version (
   ${XCB_IMDKIT_VERSION}
   VARIABLE_PREFIX XCB_IMDKIT
   VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xcb-imdkit_version.h"
   PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/XCBImdkitConfigVersion.cmake"
   SOVERSION 0
   )

ecm_configure_package_config_file (
   "${CMAKE_CURRENT_SOURCE_DIR}/XCBImdkitConfig.cmake.in"
   "${CMAKE_CURRENT_BINARY_DIR}/XCBImdkitConfig.cmake"
   INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
   PATH_VARS CMAKE_INSTALL_PREFIX
   )

install (
   FILES "${CMAKE_CURRENT_BINARY_DIR}/XCBImdkitConfig.cmake"
         "${CMAKE_CURRENT_BINARY_DIR}/XCBImdkitConfigVersion.cmake"
   DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
   COMPONENT Devel
   )

# Write out the features
feature_summary (WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
