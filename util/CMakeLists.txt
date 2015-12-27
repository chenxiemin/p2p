cmake_minimum_required (VERSION 2.6)

set(util_DIR ${CMAKE_CURRENT_LIST_DIR})

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS")
endif ()

# source file
FILE (GLOB SRC *.cpp *h)
add_library (util STATIC ${SRC})

