cmake_minimum_required(VERSION 3.2)
project(TiLog)

set(CMAKE_CXX_STANDARD 11)

option(ENABLE_SSE_4_1 "Enable SSE instructions" ON)
option(ENABLE_AVX "Enable AVX instructions" ON)


#TILOG_CUSTOMIZATION_H is used for customer to override tilog config
add_definitions(-DTILOG_CUSTOMIZATION_H="${PROJECT_SOURCE_DIR}/Test/subsystest.h")

if(ENABLE_AVX AND NOT ENABLE_SSE_4_1)
    message(WARNING "force enable sse4.1 if enable avx")
    option(ENABLE_SSE_4_1 "Enable SSE instructions" ON)
endif()
if(ENABLE_SSE_4_1)
    add_definitions(-DTILOG_ENABLE_SSE_4_1=TRUE)
else()
    add_definitions(-DTILOG_ENABLE_SSE_4_1=FALSE)
endif()
if(ENABLE_AVX)
    add_definitions(-DTILOG_ENABLE_AVX=TRUE)
else()
    add_definitions(-DTILOG_ENABLE_AVX=FALSE)
endif()


if(MSVC)
    if(ENABLE_SSE_4_1)
        add_compile_options(/arch:SSE2)
    endif()
    if(ENABLE_AVX)
        add_compile_options(/arch:AVX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(ENABLE_SSE_4_1)
        add_compile_options(-msse4.1)
    endif()
    if(ENABLE_AVX)
        add_compile_options(-mavx)
    endif()
endif()


if(MSVC)
    add_compile_options(/Zc:__cplusplus)
    add_definitions("-D _CRT_SECURE_NO_WARNINGS")

    message(STATUS "Detected MSVC compiler, adding /std options")
    if(CMAKE_CXX_STANDARD EQUAL 11)
        add_compile_options(/std:c++11)
    elseif(CMAKE_CXX_STANDARD EQUAL 14)
        add_compile_options(/std:c++14)
    elseif(CMAKE_CXX_STANDARD EQUAL 17)
        add_compile_options(/std:c++17)
    elseif(CMAKE_CXX_STANDARD EQUAL 20)
        add_compile_options(/std:c++20)
    elseif(CMAKE_CXX_STANDARD EQUAL 23)
        add_compile_options(/std:c++23)
    else()
        message(WARNING "Unsupported C++ standard for MSVC: ${CMAKE_CXX_STANDARD}")
    endif()
else()
    message(STATUS "Detected compiler GCC/clang")
endif()

add_subdirectory(depend_libs)
add_subdirectory(TiLog)
add_subdirectory(Test)


