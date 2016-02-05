cmake_minimum_required (VERSION 2.6)

set(util_DIR "../util")
find_package(util REQUIRED)

include_directories (${util_INCLUDE_DIRS})

if (CMAKE_COMPILER_IS_GNUCC)
    if (NOT CROSS_COMPILE_HI)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS")
    else ()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++0x -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_NANOSLEEP")
    endif ()
endif ()

if (CROSS_COMPILE_ANDROID)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -MMD -MP -MF -DANDROID")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpic -ffunction-sections -funwind-tables -fstack-protector -no-canonical-prefixes -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -mthumb")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -marm -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DENABLE_ANDROID_LOG")
endif ()

# source file
FILE (GLOB SRC *.cpp *h)
add_library (common STATIC ${SRC})

target_link_libraries (common util)

if (CROSS_COMPILE_HI)
    message (STATUS "Cross compile hi project...")

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_C_COMPILER arm-hisiv100nptl-linux-gcc)
    set(CMAKE_CXX_COMPILER arm-hisiv100nptl-linux-g++)
endif ()

