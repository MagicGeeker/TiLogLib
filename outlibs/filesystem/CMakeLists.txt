aux_source_directory(. DIR_LIB_SRCS)
# 生成链接库
#add_library(filesystem ${DIR_LIB_SRCS})


set(filesys_src
        filesys.cpp
        )

add_library(filesystem )
target_sources(filesystem PRIVATE  ${filesys_src})