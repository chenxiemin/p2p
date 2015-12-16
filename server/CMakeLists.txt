cmake_minimum_required (VERSION 2.6)

set(util_DIR "../util")
find_package(util REQUIRED)

set (common_DIR ../common)
find_package (common REQUIRED)

include_directories (${util_INCLUDE_DIRS})
include_directories (${common_INCLUDE_DIRS})

# source files
FILE (GLOB SRC ./*.cpp)
add_executable (p2pserver ${SRC})

target_link_libraries (p2pserver util)
target_link_libraries (p2pserver common)

# linked library
# if (MSVC)
# 	set(LD_PATH "${PROJECT_SOURCE_DIR}/third-party/lib/win32")
# 	set(LD_PATH "${PROJECT_SOURCE_DIR}/third-party/lib/linux")
# endif (MSVC)
# 
# target_link_libraries (test-player LINK_PUBLIC player)
if (MSVC)
	target_link_libraries(p2pserver ws2_32)
	target_link_libraries(p2pserver wsock32)
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS -g -O0")
	target_link_libraries(p2pserver pthread)
endif ()
