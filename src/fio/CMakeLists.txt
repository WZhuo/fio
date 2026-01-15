set(FIO_SOURCES
    sem.cc
)

add_executable(fio ${FIO_SOURCES} fio.cc)
target_link_libraries(fio ${LIBS})