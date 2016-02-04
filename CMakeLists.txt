cmake_minimum_required (VERSION 2.6)
project (p2p)

if (CROSS_COMPILE_ANDROID)
    message (STATUS "Cross compile android project...")

    set(CMAKE_SYSTEM_NAME Android)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_C_COMPILER arm-linux-androideabi-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-androideabi-g++)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pie -fPIE -march=armv7-a -mfloat-abi=softfp")
endif ()

add_subdirectory (./util)
add_subdirectory (./common)
add_subdirectory (./client)
add_subdirectory (./server)

