cmake_minimum_required(VERSION 3.2)
project(TiLog)

set(CMAKE_CXX_STANDARD 11)

#TILOG_CUSTOMIZATION_H is used for customer to override tilog config
add_definitions(-DTILOG_CUSTOMIZATION_H="${PROJECT_SOURCE_DIR}/Test/subsystest.h")

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
    add_definitions("-D _CRT_SECURE_NO_WARNINGS")
    add_compile_options(/std:c++11)
endif()

add_subdirectory(depend_libs)
add_subdirectory(TiLog)
add_subdirectory(Test)


