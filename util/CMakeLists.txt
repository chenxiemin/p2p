cmake_minimum_required (VERSION 2.6)

set(util_DIR ${CMAKE_CURRENT_LIST_DIR})

# source file
FILE (GLOB SRC *.cpp *h)
add_library (util STATIC ${SRC})
