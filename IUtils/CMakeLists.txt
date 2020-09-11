aux_source_directory(. DIR_LIB_SRCS)
# 生成链接库
add_library(IUtils ${DIR_LIB_SRCS} )

#file(GLOB IU_ALL_FILES "*.cpp")
#file(GLOB IU_ALL_FILES "*.h")
#file(GLOB IU_ALL_FILES "*.hpp")
#
#foreach(IU_FILE ${IU_ALL_FILES})
#    get_filename_component(IU_FILE ${IU_FILE} NAME)
#    string(REPLACE "_iutils.cc" "" IU_TARGET ${IU_FILE})
#    message("  - ${IU_FILE} --> ${IU_TARGET}")
#    add_executable(${IU_TARGET} ${IU_FILE})
#    target_link_libraries(${IU_TARGET}
#            ${LIBS_TO_LINK}
#            )
#endforeach(IU_FILE ${IU_ALL_FILES})