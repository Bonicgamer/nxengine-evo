cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_definitions("-fPIC")

project(nx C CXX)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 6)
set (nx_VERSION_RELEASE 5)
set (nx_APP_ID org.nxengine.nxengine_evo)

include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

include_directories("${nx_SOURCE_DIR}/deps")

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("debug mode")
    add_definitions(-DTRACE_SCRIPT)
    add_definitions(-DDEBUG)
ENDIF()

add_definitions("-Wall")

add_subdirectory(src)
