aux_source_directory(. DIR_LIB_SRCS)

add_executable(main ${DIR_LIB_SRCS} )

add_dependencies(main TiLog)
add_dependencies(main Catch)

target_link_libraries(main PUBLIC TiLog)
target_link_libraries(main PUBLIC Catch)
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(main  PUBLIC Threads::Threads)
endif()