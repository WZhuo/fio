# Test programs CMakeLists.txt

# stest (smalloc test)
add_executable(stest
    stest.c
    ../gettime.c ../fio_sem.c ../pshared.c ../smalloc.c
    log.c debug.c arch.c
)
target_link_libraries(stest pthread)

# ieee754 test
add_executable(ieee754
    ieee754.c
    ../lib/ieee754.c
)

# fio-genzipf
add_executable(fio-genzipf
    genzipf.c
    log.c
    ../lib/ieee754.c ../lib/rand.c ../lib/pattern.c ../lib/zipf.c
    ../lib/strntol.c ../lib/gauss.c
    ../oslib/strcasestr.c ../oslib/strndup.c
)
target_link_libraries(fio-genzipf m)

# axmap test
add_executable(axmap
    axmap.c
    ../lib/lfsr.c ../lib/axmap.c
)

# lfsr-test
add_executable(lfsr-test
    lfsr-test.c
    ../lib/lfsr.c ../gettime.c ../fio_sem.c ../pshared.c
    log.c debug.c arch.c
)
target_link_libraries(lfsr-test pthread)

# gen-rand
add_executable(gen-rand
    gen-rand.c
    log.c debug.c
    ../lib/rand.c ../lib/pattern.c ../lib/strntol.c
    ../oslib/strcasestr.c ../oslib/strndup.c
)
target_link_libraries(gen-rand m)

# fio-dedupe (requires zlib)
if(ZLIB_FOUND)
    add_executable(fio-dedupe
        dedupe.c
        ../lib/rbtree.c log.c ../fio_sem.c ../pshared.c ../smalloc.c
        ../gettime.c ../crc/md5.c ../lib/memalign.c ../lib/bloom.c
        debug.c ../crc/xxhash.c arch.c ../crc/murmur3.c
        ../crc/crc32c.c ../crc/crc32c-intel.c ../crc/crc32c-arm64.c
        ../crc/fnv.c
    )
    target_link_libraries(fio-dedupe pthread z)
endif()

# fio-verify-state
add_executable(fio-verify-state
    verify-state.c
    log.c
    ../crc/crc32c.c ../crc/crc32c-intel.c ../crc/crc32c-arm64.c
    debug.c
)

# memlock test
add_executable(memlock memlock.c log.c)

# time-test
add_executable(time-test time-test.c)

# io_uring test (Linux only)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(io_uring
        io_uring.c
        ../lib/rand.c ../lib/pattern.c ../lib/strntol.c
    )
    target_link_libraries(io_uring m)
endif()

# Install test programs
install(TARGETS
    stest ieee754 fio-genzipf axmap lfsr-test gen-rand fio-verify-state
    DESTINATION bin
    OPTIONAL
)

if(ZLIB_FOUND)
    install(TARGETS fio-dedupe DESTINATION bin OPTIONAL)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(TARGETS io_uring DESTINATION bin OPTIONAL)
endif()
