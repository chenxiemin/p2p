cmake_minimum_required (VERSION 2.6)

set(util_DIR ${CMAKE_CURRENT_LIST_DIR})

if (CMAKE_COMPILER_IS_GNUCC)
    if (NOT CROSS_COMPILE_HI)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS")
    else ()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++0x -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_NANOSLEEP")
    endif ()
endif ()

# source file
FILE (GLOB SRC *.cpp *h)
add_library (util STATIC ${SRC})

if (CROSS_COMPILE_HI)
    message (STATUS "Cross compile hi project...")

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_C_COMPILER arm-hisiv100nptl-linux-gcc)
    set(CMAKE_CXX_COMPILER arm-hisiv100nptl-linux-g++)
endif ()

