cmake_minimum_required(VERSION 3.10)
project(fio VERSION 3.0 LANGUAGES C CXX)

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

# Detect wordsize
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WORDSIZE 64)
    set(BITS_PER_LONG 64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(WORDSIZE 32)
    set(BITS_PER_LONG 32)
endif()

# Detect endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

# Set seed buckets based on architecture
if(BITS_PER_LONG EQUAL 64)
    set(CONFIG_SEED_BUCKETS 64)
else()
    set(CONFIG_SEED_BUCKETS 32)
endif()

# Compiler flags (without config-host.h for now since it doesn't exist yet)
# set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS}")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wwrite-strings -Wall -Wdeclaration-after-statement")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DFIO_INTERNAL")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIO_INC_DEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIO_VERSION='\"${FIO_VERSION}\"'")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ffast-math")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBITS_PER_LONG=${BITS_PER_LONG}")

if(NOT DISABLE_OPT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()

if(BUILD_NATIVE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-builtin-stpcpy")
endif()

if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
    endif()
endif()

include(cmake/config.cmake)

# Now create config-host.h header after all feature detection
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/config-host.h.in"
    "${CMAKE_BINARY_DIR}/config-host.h"
)

# Add the config-host.h include flag AFTER generating it
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_BINARY_DIR}/config-host.h")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR})

# Libraries
set(LIBS m)

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND LIBS pthread dl)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "DragonFly")
    list(APPEND LIBS pthread rt)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    list(APPEND LIBS pthread)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    list(APPEND LIBS pthread rt)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND LIBS pthread dl)
elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    list(APPEND LIBS pthread dl)
    add_definitions(-D__EXTENSIONS__)
endif()

# Check for zlib
find_package(ZLIB)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    list(APPEND LIBS ${ZLIB_LIBRARIES})
    add_definitions(-DCONFIG_ZLIB)
endif()

add_subdirectory(src)

# Configuration summary
message(STATUS "========================================")
message(STATUS "FIO Configuration Summary")
message(STATUS "========================================")
message(STATUS "Version: ${FIO_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
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
