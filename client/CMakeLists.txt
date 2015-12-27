cmake_minimum_required (VERSION 2.6)

set(util_DIR "../util")
find_package(util REQUIRED)

set (common_DIR ../common)
find_package (common REQUIRED)

include_directories (${util_INCLUDE_DIRS})
include_directories (${common_INCLUDE_DIRS})


# source files
FILE (GLOB SRC ./*.cpp)
add_executable (p2pclient ${SRC})

target_link_libraries (p2pclient util)
target_link_libraries (p2pclient common)

if (MSVC)
	target_link_libraries(p2pclient ws2_32)
	target_link_libraries(p2pclient wsock32)
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
    if (NOT CROSS_COMPILE_HI)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS")
    else ()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++0x -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_NANOSLEEP")
    endif ()

    if (NOT CROSS_COMPILE_ANDROID)
        target_link_libraries(p2pclient pthread)
    endif ()
endif ()

if (CROSS_COMPILE_ANDROID)
    message (STATUS "Cross compile android project...")

    set(CMAKE_SYSTEM_NAME Android)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_C_COMPILER arm-linux-androideabi-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-androideabi-g++)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pie -fPIE -march=armv7-a -mfloat-abi=softfp")
    target_link_libraries(p2pclient log)
endif ()

if (CROSS_COMPILE_HI)
    message (STATUS "Cross compile hi project...")

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_C_COMPILER arm-hisiv100nptl-linux-gcc)
    set(CMAKE_CXX_COMPILER arm-hisiv100nptl-linux-g++)
endif ()

