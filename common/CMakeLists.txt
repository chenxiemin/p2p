cmake_minimum_required (VERSION 2.6)

set(util_DIR "../util")
find_package(util REQUIRED)

include_directories (${util_INCLUDE_DIRS})

# source file
FILE (GLOB SRC *.cpp *h)
add_library (common STATIC ${SRC})

target_link_libraries (common util)
