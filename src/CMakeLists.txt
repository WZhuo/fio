# Source files
set(FIO_SOURCES
    gettime.c ioengines.c init.c stat.c log.c time.c filesetup.c
    eta.c verify.c memory.c io_u.c parse.c fio_sem.c rwlock.c
    pshared.c options.c fio_shared_sem.c
    smalloc.c filehash.c profile.c debug.c
    server.c client.c iolog.c backend.c libfio.c flow.c cconv.c
    gettime-thread.c helpers.c json.c idletime.c td_error.c
    io_u_queue.c filelock.c
    workqueue.c rate-submit.c optgroup.c helper_thread.c
    steadystate.c zone-dist.c zbd.c dedupe.c dataplacement.c
    sprandom.c
)

# Engine sources
list(APPEND FIO_SOURCES
    engines/cpu.c
    engines/mmap.c
    engines/sync.c
    engines/null.c
    engines/net.c
    engines/ftruncate.c
    engines/fileoperations.c
    engines/exec.c
)

# Profile sources
list(APPEND FIO_SOURCES
    profiles/tiobench.c
    profiles/act.c
)

# CRC sources
file(GLOB CRC_SOURCES crc/*.c)
list(APPEND FIO_SOURCES ${CRC_SOURCES})

# Lib sources
file(GLOB LIB_SOURCES lib/*.c)
list(APPEND FIO_SOURCES ${LIB_SOURCES})

# OS library sources
list(APPEND FIO_SOURCES oslib/asprintf.c)

# Conditional oslib sources based on what the system provides
if(NOT HAVE_STRSEP)
    list(APPEND FIO_SOURCES oslib/strsep.c)
endif()
if(NOT HAVE_STRCASESTR)
    list(APPEND FIO_SOURCES oslib/strcasestr.c)
endif()
if(NOT HAVE_STRLCAT)
    list(APPEND FIO_SOURCES oslib/strlcat.c)
endif()
if(NOT HAVE_STRNDUP)
    list(APPEND FIO_SOURCES oslib/strndup.c)
endif()
if(NOT HAVE_GETOPT_LONG)
    list(APPEND FIO_SOURCES oslib/getopt_long.c)
endif()

# Platform-specific sources
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND FIO_SOURCES
        diskutil.c fifo.c blktrace.c cgroup.c trim.c
        engines/sg.c engines/io_uring.c engines/nvme.c
        oslib/linux-dev-lookup.c
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "DragonFly")
    list(APPEND FIO_SOURCES trim.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND FIO_SOURCES os/mac/posix.c)
endif()
if(NOT HAVE_STATX)
    list(APPEND FIO_SOURCES oslib/statx.c)
endif()

# Main fio executable
add_executable(fio ${FIO_SOURCES} fio.c)
target_link_libraries(fio ${LIBS})

# GFIO (if enabled)
if(BUILD_GFIO)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-2.0)
    
    set(GFIO_SOURCES ${FIO_SOURCES}
        gfio.c graph.c tickmarks.c ghelpers.c goptions.c gerror.c
        gclient.c gcompat.c cairo_text_helpers.c printing.c
    )
    
    add_executable(gfio ${GFIO_SOURCES})
    target_include_directories(gfio PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_libraries(gfio ${LIBS} ${GTK_LIBRARIES})
    target_compile_options(gfio PRIVATE ${GTK_CFLAGS_OTHER})
endif()

# Test programs
add_subdirectory(t)

# Install rules
install(TARGETS fio DESTINATION bin)
if(BUILD_GFIO)
    install(TARGETS gfio DESTINATION bin)
endif()

install(FILES fio.1 DESTINATION share/man/man1)
install(FILES tools/fio_generate_plots.1 DESTINATION share/man/man1)
install(FILES tools/hist/fiologparser_hist.py.1 DESTINATION share/man/man1)

install(PROGRAMS
    tools/fio_generate_plots
    tools/plot/fio2gnuplot
    tools/genfio
    tools/fiologparser.py
    tools/hist/fiologparser_hist.py
    tools/hist/fio-histo-log-pctiles.py
    tools/fio_jsonplus_clat2csv
    DESTINATION bin
)

install(FILES tools/plot/*.gpm DESTINATION share/fio)
