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
    if (NOT CROSS_COMPILE_HI)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++11 -D__STDC_CONSTANT_MACROS")
    else ()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++0x -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_NANOSLEEP")
    endif ()
	target_link_libraries(p2pserver pthread)
endif ()

if (CROSS_COMPILE_HI)
    message (STATUS "Cross compile hi project...")

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_C_COMPILER arm-hisiv100nptl-linux-gcc)
    set(CMAKE_CXX_COMPILER arm-hisiv100nptl-linux-g++)
endif ()

