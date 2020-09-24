aux_source_directory(. DIR_LIB_SRCS)
# 生成链接库
add_library(EzLog ${DIR_LIB_SRCS} )

add_dependencies(EzLog IUtils)
target_link_libraries(EzLog PUBLIC IUtils)