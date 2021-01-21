aux_source_directory(. DIR_LIB_SRCS)
# 生成链接库
add_library(TiLog ${DIR_LIB_SRCS} )

add_dependencies(TiLog IUtils)
target_link_libraries(TiLog PUBLIC IUtils)