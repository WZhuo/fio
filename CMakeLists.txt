cmake_minimum_required(VERSION 3.10)
project(fio VERSION 3.0 LANGUAGES CXX)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)

# Options
option(BUILD_GFIO "Build gfio GUI" OFF)
option(BUILD_STATIC "Build static binary" OFF)
option(BUILD_NATIVE "Build with -march=native" OFF)
option(DYNAMIC_ENGINES "Build engines as dynamic libraries" OFF)
option(DISABLE_OPT "Disable optimizations" OFF)
option(NO_SHM "Disable shared memory" OFF)

# Set CXX standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Generate version file
execute_process(
    COMMAND ${CMAKE_SOURCE_DIR}/FIO-VERSION-GEN
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Read version from FIO-VERSION-FILE
if(EXISTS "${CMAKE_SOURCE_DIR}/FIO-VERSION-FILE")
    file(STRINGS "${CMAKE_SOURCE_DIR}/FIO-VERSION-FILE" FIO_VERSION_LINE REGEX "^FIO_VERSION")
    string(REGEX REPLACE ".*=\\s*(.*)" "\\1" FIO_VERSION "${FIO_VERSION_LINE}")
    string(STRIP "${FIO_VERSION}" FIO_VERSION)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFIO_VERSION='\"${FIO_VERSION}\"'")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR})

# Libraries
set(LIBS m)

# Check for zlib
find_package(ZLIB)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    list(APPEND LIBS ${ZLIB_LIBRARIES})
    add_definitions(-DCONFIG_ZLIB)
endif()

add_subdirectory(src/fio)

# Configuration summary
message(STATUS "========================================")
message(STATUS "FIO Configuration Summary")
message(STATUS "========================================")
message(STATUS "Version: ${FIO_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CXX Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Build static: ${BUILD_STATIC}")
message(STATUS "Build gfio: ${BUILD_GFIO}")
message(STATUS "Build native: ${BUILD_NATIVE}")
message(STATUS "Dynamic engines: ${DYNAMIC_ENGINES}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
if(ZLIB_FOUND)
    message(STATUS "zlib: enabled")
else()
    message(STATUS "zlib: disabled")
endif()
message(STATUS "========================================")