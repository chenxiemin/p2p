cmake_minimum_required (VERSION 2.6)

include_directories("../common")

# source files
FILE (GLOB SRC ./*.cpp ../common/*.cpp ../common/*.h)
add_executable (p2pclient ${SRC})

if (MSVC)
	target_link_libraries(p2pclient ws2_32)
	target_link_libraries(p2pclient wsock32)
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS")
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
