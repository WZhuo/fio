# Feature detection
check_function_exists(posix_fadvise HAVE_POSIX_FADVISE)
check_function_exists(posix_fallocate HAVE_POSIX_FALLOCATE)
check_function_exists(fdatasync HAVE_FDATASYNC)
check_function_exists(pipe HAVE_PIPE)
check_function_exists(pipe2 HAVE_PIPE2)
check_function_exists(pread HAVE_PREAD)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)

check_symbol_exists(sync_file_range "fcntl.h;linux/fs.h" HAVE_SYNC_FILE_RANGE)

# Check for libaio (Linux AIO)
check_library_exists(aio io_setup "" HAVE_LIBAIO)
if(HAVE_LIBAIO)
    set(CONFIG_LIBAIO 1)
endif()

# Check for posix AIO
check_function_exists(aio_read HAVE_POSIX_AIO)
if(HAVE_POSIX_AIO)
    set(CONFIG_POSIXAIO 1)
endif()

# Check for posix AIO fsync
if(HAVE_POSIX_AIO)
    check_c_source_compiles("
    #include <fcntl.h>
    #include <aio.h>
    int main(void) {
        struct aiocb cb;
        return aio_fsync(O_SYNC, &cb);
    }
    " HAVE_POSIX_AIO_FSYNC)
    if(HAVE_POSIX_AIO_FSYNC)
        set(CONFIG_POSIXAIO_FSYNC 1)
    endif()
endif()

# Check for solaris AIO
check_library_exists(aio aioread "" HAVE_SOLARIS_AIO)
if(HAVE_SOLARIS_AIO)
    set(CONFIG_SOLARISAIO 1)
endif()

# Check for POSIX pshared
check_c_source_compiles("
#include <unistd.h>
int main(void) {
#if defined(_POSIX_THREAD_PROCESS_SHARED) && ((_POSIX_THREAD_PROCESS_SHARED + 0) > 0)
    return 0;
#else
    #error \"_POSIX_THREAD_PROCESS_SHARED is unsupported\"
#endif
}
" HAVE_POSIX_PSHARED)
if(HAVE_POSIX_PSHARED)
    set(CONFIG_PSHARED 1)
endif()

# Check for pthread_condattr_setclock
check_c_source_compiles("
#include <pthread.h>
int main(void) {
    pthread_condattr_t condattr;
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    return 0;
}
" HAVE_PTHREAD_CONDATTR_SETCLOCK)
if(HAVE_PTHREAD_CONDATTR_SETCLOCK)
    set(CONFIG_PTHREAD_CONDATTR_SETCLOCK 1)
endif()

# Check for inet_aton
check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
int main(int argc, char **argv) {
    struct in_addr in;
    return inet_aton(NULL, &in);
}
" HAVE_INET_ATON)
if(HAVE_INET_ATON)
    set(CONFIG_INET_ATON 1)
endif()

# Check for socklen_t
check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif
int main(int argc, char **argv) {
    socklen_t len = 0;
    return len;
}
" HAVE_SOCKLEN_T)
if(HAVE_SOCKLEN_T)
    set(CONFIG_SOCKLEN_T 1)
endif()

# Check for __thread support
check_c_source_compiles("
#include <stdio.h>
static __thread int ret;
int main(int argc, char **argv) {
    return ret;
}
" HAVE_TLS_THREAD)
if(HAVE_TLS_THREAD)
    set(CONFIG_TLS_THREAD 1)
endif()

# Check for TCP_NODELAY
check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#else
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif
int main(int argc, char **argv) {
    return getsockopt(0, 0, TCP_NODELAY, NULL, NULL);
}
" HAVE_TCP_NODELAY)
if(NOT HAVE_TCP_NODELAY)
    check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#else
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif
int main(int argc, char **argv) {
    return getsockopt(0, 0, TCP_NODELAY, NULL, NULL);
}
" HAVE_TCP_NODELAY_WS2_32)
    if(HAVE_TCP_NODELAY_WS2_32)
        set(HAVE_TCP_NODELAY 1)
    endif()
endif()
if(HAVE_TCP_NODELAY)
    set(CONFIG_TCP_NODELAY 1)
endif()

# Check for pthread functions
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_function_exists(pthread_sigmask HAVE_PTHREAD_SIGMASK)
check_function_exists(pthread_getaffinity_np HAVE_PTHREAD_GETAFFINITY)
set(CMAKE_REQUIRED_LIBRARIES)

# Check sched_setaffinity arguments
check_c_source_compiles("
#include <sched.h>
int main() {
    cpu_set_t mask;
    return sched_setaffinity(0, sizeof(mask), &mask);
}
" HAVE_3ARG_AFFINITY)

if(NOT HAVE_3ARG_AFFINITY)
    check_c_source_compiles("
    #include <sched.h>
    int main() {
        cpu_set_t mask;
        return sched_setaffinity(0, &mask);
    }
    " HAVE_2ARG_AFFINITY)
endif()

# Check for clock functions
check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
if(NOT HAVE_CLOCK_GETTIME)
    check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME_RT)
    if(HAVE_CLOCK_GETTIME_RT)
        set(HAVE_CLOCK_GETTIME TRUE)
    endif()
endif()

check_symbol_exists(CLOCK_MONOTONIC "time.h" HAVE_CLOCK_MONOTONIC)

# Check atomic operations
check_c_source_compiles("
#include <stdatomic.h>
int main() {
    atomic_int v;
    atomic_fetch_add(&v, 1);
    return 0;
}
" HAVE_C11_ATOMICS)

check_c_source_compiles("
#include <inttypes.h>
int main() {
    uint64_t v = 0;
    return __sync_fetch_and_add(&v, 1);
}
" HAVE_SYNC_FETCH_AND_ADD)

check_c_source_compiles("
int main() {
    __sync_synchronize();
    return 0;
}
" HAVE_SYNC_SYNCHRONIZE)

check_c_source_compiles("
#include <inttypes.h>
int main() {
    uint64_t v = 0, old = 0, new = 1;
    return __sync_val_compare_and_swap(&v, old, new);
}
" HAVE_SYNC_VAL_COMPARE_AND_SWAP)

# Check for asprintf/vasprintf
check_function_exists(asprintf HAVE_ASPRINTF)
check_function_exists(vasprintf HAVE_VASPRINTF)

# Check for string functions that might conflict with system headers
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strsep HAVE_STRSEP)
check_function_exists(strcasestr HAVE_STRCASESTR)
check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(getopt_long HAVE_GETOPT_LONG)

# Set configuration defines based on detected features
if(HAVE_POSIX_FADVISE)
    set(CONFIG_POSIX_FADVISE 1)
endif()
if(HAVE_POSIX_FALLOCATE)
    set(CONFIG_POSIX_FALLOCATE 1)
endif()
if(HAVE_FDATASYNC)
    set(CONFIG_FDATASYNC 1)
endif()
if(HAVE_PIPE)
    set(CONFIG_PIPE 1)
endif()
if(HAVE_PIPE2)
    set(CONFIG_PIPE2 1)
endif()
if(HAVE_PREAD)
    set(CONFIG_PREAD 1)
endif()
if(HAVE_GETTIMEOFDAY)
    set(CONFIG_GETTIMEOFDAY 1)
endif()
if(HAVE_SYNC_FILE_RANGE)
    set(CONFIG_SYNC_FILE_RANGE 1)
endif()
if(HAVE_CLOCK_GETTIME)
    set(CONFIG_CLOCK_GETTIME 1)
endif()
if(HAVE_CLOCK_MONOTONIC)
    set(CONFIG_CLOCK_MONOTONIC 1)
endif()
if(HAVE_PTHREAD_SIGMASK)
    set(CONFIG_PTHREAD_SIGMASK 1)
endif()
if(HAVE_PTHREAD_GETAFFINITY)
    set(CONFIG_PTHREAD_GETAFFINITY 1)
endif()
if(HAVE_3ARG_AFFINITY)
    set(CONFIG_SCHED_SETAFFINITY_3_ARG 1)
endif()
if(HAVE_2ARG_AFFINITY)
    set(CONFIG_SCHED_SETAFFINITY_2_ARG 1)
endif()
if(HAVE_SYNC_FETCH_AND_ADD)
    set(CONFIG_SFAA 1)
endif()
if(HAVE_SYNC_SYNCHRONIZE)
    set(CONFIG_SYNC_SYNC 1)
endif()
if(HAVE_SYNC_VAL_COMPARE_AND_SWAP)
    set(CONFIG_CMP_SWAP 1)
endif()
if(HAVE_ASPRINTF)
    set(CONFIG_HAVE_ASPRINTF 1)
endif()
if(HAVE_VASPRINTF)
    set(CONFIG_HAVE_VASPRINTF 1)
endif()
if(HAVE_STRLCAT)
    set(CONFIG_STRLCAT 1)
endif()
if(HAVE_STRSEP)
    set(CONFIG_STRSEP 1)
endif()
if(HAVE_STRCASESTR)
    set(CONFIG_STRCASESTR 1)
endif()
if(HAVE_STRNDUP)
    set(CONFIG_HAVE_STRNDUP 1)
endif()
if(HAVE_GETOPT_LONG)
    set(CONFIG_GETOPT_LONG_ONLY 1)
endif()
if(IS_BIG_ENDIAN)
    set(CONFIG_BIG_ENDIAN 1)
endif()
if(NO_SHM)
    set(CONFIG_NO_SHM 1)
endif()

# Check for Linux-specific features
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    check_c_source_compiles("
    #include <fcntl.h>
    #include <linux/falloc.h>
    int main() {
        return fallocate(0, 0, 0, 0);
    }
    " HAVE_LINUX_FALLOCATE)
    if(HAVE_LINUX_FALLOCATE)
        set(CONFIG_LINUX_FALLOCATE 1)
    endif()
    
    check_c_source_compiles("
    #include <fcntl.h>
    int main() {
        return splice(0, NULL, 0, NULL, 0, 0);
    }
    " HAVE_LINUX_SPLICE)
    if(HAVE_LINUX_SPLICE)
        set(CONFIG_LINUX_SPLICE 1)
    endif()
    
    # Check for ext4 move extent
    check_symbol_exists(MOVE_EXT_IOC_EXT4_MOVE_EXT "sys/ioctl.h" HAVE_EXT4_ME)
    if(HAVE_EXT4_ME OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CONFIG_LINUX_EXT4_MOVE_EXTENT 1)
    endif()
endif()

# Check for statx support
check_c_source_compiles("
#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
int main(int argc, char **argv) {
    struct statx st;
    return statx(-1, *argv, 0, 0, &st);
}
" HAVE_STATX)

if(HAVE_STATX)
    set(CONFIG_HAVE_STATX 1)
endif()

# Check for RUSAGE_THREAD
check_c_source_compiles("
#include <sys/time.h>
#include <sys/resource.h>
int main(int argc, char **argv) {
    struct rusage ru;
    getrusage(RUSAGE_THREAD, &ru);
    return 0;
}
" HAVE_RUSAGE_THREAD)
if(HAVE_RUSAGE_THREAD)
    set(CONFIG_RUSAGE_THREAD 1)
endif()

# Check for SCHED_IDLE
check_c_source_compiles("
#include <sched.h>
int main(int argc, char **argv) {
    struct sched_param p = {};
    return sched_setscheduler(0, SCHED_IDLE, &p);
}
" HAVE_SCHED_IDLE)
if(HAVE_SCHED_IDLE)
    set(CONFIG_SCHED_IDLE 1)
endif()

# Check for vsock
check_c_source_compiles("
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
int main(int argc, char **argv) {
    return socket(AF_VSOCK, SOCK_STREAM, 0);
}
" HAVE_VSOCK)
if(HAVE_VSOCK)
    set(CONFIG_VSOCK 1)
endif()

# Check for SO_SNDBUF
check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#else
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif
int main(int argc, char **argv) {
    setsockopt(0, SOL_SOCKET, SO_SNDBUF, NULL, 0);
    setsockopt(0, SOL_SOCKET, SO_RCVBUF, NULL, 0);
    return 0;
}
" HAVE_SO_SNDBUF)
if(HAVE_SO_SNDBUF)
    set(CONFIG_NET_WINDOWSIZE 1)
endif()

# Check for TCP_MAXSEG
check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#else
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
int main(int argc, char **argv) {
    return setsockopt(0, IPPROTO_TCP, TCP_MAXSEG, NULL, 0);
}
" HAVE_NET_MSS)
if(HAVE_NET_MSS)
    set(CONFIG_NET_MSS 1)
endif()

# Check for RLIMIT_MEMLOCK
check_c_source_compiles("
#include <sys/time.h>
#include <sys/resource.h>
int main(int argc, char **argv) {
    struct rlimit rl;
    return getrlimit(RLIMIT_MEMLOCK, &rl);
}
" HAVE_RLIMIT_MEMLOCK)
if(HAVE_RLIMIT_MEMLOCK)
    set(CONFIG_RLIMIT_MEMLOCK 1)
endif()

# Check for pwritev/preadv
check_c_source_compiles("
#include <stdio.h>
#include <sys/uio.h>
int main(int argc, char **argv) {
    struct iovec iov[1] = {};
    return pwritev(0, iov, 1, 0) + preadv(0, iov, 1, 0);
}
" HAVE_PWRITEV)
if(HAVE_PWRITEV)
    set(CONFIG_PWRITEV 1)
endif()

# Check for pwritev2/preadv2
check_c_source_compiles("
#include <stdio.h>
#include <sys/uio.h>
int main(int argc, char **argv) {
    struct iovec iov[1] = {};
    return pwritev2(0, iov, 1, 0, 0) + preadv2(0, iov, 1, 0, 0);
}
" HAVE_PWRITEV2)
if(HAVE_PWRITEV2)
    set(CONFIG_PWRITEV2 1)
endif()

# Check for IPv6 support
check_c_source_compiles("
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <stdio.h>
int main(int argc, char **argv) {
    struct addrinfo hints = {};
    struct in6_addr addr = in6addr_any;
    int ret;
    ret = getaddrinfo(NULL, NULL, &hints, NULL);
    freeaddrinfo(NULL);
    printf(\"%s %d\\n\", gai_strerror(ret), addr.s6_addr[0]);
    return 0;
}
" HAVE_IPV6)
if(HAVE_IPV6)
    set(CONFIG_IPV6 1)
endif()

# Check for getopt_long_only
check_c_source_compiles("
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
int main(int argc, char **argv) {
    int c = getopt_long_only(argc, argv, \"\", NULL, NULL);
    return c;
}
" HAVE_GETOPT_LONG_ONLY)
if(HAVE_GETOPT_LONG_ONLY)
    set(CONFIG_GETOPT_LONG_ONLY 1)
endif()

# Check for libverbs (RDMA)
check_c_source_compiles("
#include <infiniband/verbs.h>
int main(int argc, char **argv) {
    struct ibv_pd *pd = ibv_alloc_pd(NULL);
    return pd != NULL;
}
" HAVE_LIBVERBS)
if(HAVE_LIBVERBS)
    set(CONFIG_LIBVERBS 1)
endif()

# Check for rdmacm
check_c_source_compiles("
#include <stdio.h>
#include <rdma/rdma_cma.h>
int main(int argc, char **argv) {
    rdma_destroy_qp(NULL);
    return 0;
}
" HAVE_RDMACM)
if(HAVE_RDMACM)
    set(CONFIG_RDMACM 1)
endif()

# Check for libnuma
check_library_exists(numa numa_available "" HAVE_LIBNUMA)
if(HAVE_LIBNUMA)
    set(CONFIG_LIBNUMA 1)
endif()

# Check for libnuma v2 API
if(HAVE_LIBNUMA)
    check_c_source_compiles("
    #include <numa.h>
    int main(int argc, char **argv) {
        struct bitmask *mask = numa_parse_nodestring(NULL);
        return mask->size == 0;
    }
    " HAVE_LIBNUMA_V2)
    if(HAVE_LIBNUMA_V2)
        set(CONFIG_LIBNUMA_V2 1)
    endif()
endif()

# Check for ASharedMemory_create (Android)
check_c_source_compiles("
#include <android/sharedmem.h>
int main(int argc, char **argv) {
    return ASharedMemory_create(\"\", 0);
}
" HAVE_ASHAREDMEMORY_CREATE)
if(HAVE_ASHAREDMEMORY_CREATE)
    set(CONFIG_ASHAREDMEMORY_CREATE 1)
endif()

# Check for setvbuf
check_c_source_compiles("
#include <stdio.h>
int main(int argc, char **argv) {
    FILE *f = NULL;
    char buf[80];
    setvbuf(f, buf, _IOFBF, sizeof(buf));
    return 0;
}
" HAVE_SETVBUF)
if(HAVE_SETVBUF)
    set(CONFIG_SETVBUF 1)
endif()
# Check for getmntent
check_c_source_compiles("
#include <stdio.h>
#include <mntent.h>
int main(int argc, char **argv) {
    FILE *mtab = setmntent(NULL, \"r\");
    struct mntent *mnt = getmntent(mtab);
    endmntent(mtab);
    return mnt != NULL;
}
" HAVE_GETMNTENT)
if(HAVE_GETMNTENT)
    set(CONFIG_GETMNTENT 1)
endif()

# Check for getmntinfo
check_c_source_compiles("
#include <stdio.h>
#include <sys/param.h>
#include <sys/mount.h>
int main(int argc, char **argv) {
    struct statfs *st;
    return getmntinfo(&st, MNT_NOWAIT);
}
" HAVE_GETMNTINFO)
if(HAVE_GETMNTINFO)
    set(CONFIG_GETMNTINFO 1)
endif()

# Check for getmntinfo_statvfs
if(NOT HAVE_GETMNTINFO)
    check_c_source_compiles("
    #include <stdio.h>
    #include <sys/statvfs.h>
    int main(int argc, char **argv) {
        struct statvfs *st;
        return getmntinfo(&st, MNT_NOWAIT);
    }
    " HAVE_GETMNTINFO_STATVFS)
    if(HAVE_GETMNTINFO_STATVFS)
        set(CONFIG_GETMNTINFO_STATVFS 1)
    endif()
endif()

# Check for _Static_assert
check_c_source_compiles("
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
struct foo { int a, b; };
int main(int argc, char **argv) {
    _Static_assert(offsetof(struct foo, a) == 0 , \"Check\");
    return 0;
}
" HAVE_STATIC_ASSERT)
if(HAVE_STATIC_ASSERT)
    set(CONFIG_STATIC_ASSERT 1)
endif()

# Check for bool
check_c_source_compiles("
#include <stdbool.h>
int main(int argc, char **argv) {
    bool var = true;
    return var != false;
}
" HAVE_BOOL)
if(HAVE_BOOL)
    set(CONFIG_HAVE_BOOL 1)
endif()

# Check for ARMv8 CRC+Crypto
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(CMAKE_REQUIRED_FLAGS "-march=armv8-a+crc+crypto")
    check_c_source_compiles("
    #if __linux__
    #include <arm_acle.h>
    #include <arm_neon.h>
    #include <sys/auxv.h>
    #endif

    int main(void)
    {
      /* Can we also do a runtime probe? */
    #if __linux__
      return getauxval(AT_HWCAP);
    #elif defined(__APPLE__)
      return 0;
    #else
    # error \"Don't know how to do runtime probe for ARM CRC32c\"
    #endif
    }
    " HAVE_ARMV8_CRC_CRYPTO)
    set(CMAKE_REQUIRED_FLAGS) # Clear flags
    if(HAVE_ARMV8_CRC_CRYPTO)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc+crypto")
        set(ARCH_HAVE_CRC_CRYPTO 1)
    endif()
endif()
