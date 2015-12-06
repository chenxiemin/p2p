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
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O3 -std=gnu++11 -D__STDC_CONSTANT_MACROS")
	target_link_libraries(p2pclient pthread)
endif ()

# linked library
# if (MSVC)
# 	set(LD_PATH "${PROJECT_SOURCE_DIR}/third-party/lib/win32")
# 	set(LD_PATH "${PROJECT_SOURCE_DIR}/third-party/lib/linux")
# endif (MSVC)
# 
# target_link_libraries (test-player LINK_PUBLIC player)
# if (MSVC)
#     target_link_libraries(test-player ${LD_PATH}/SDL2)
#     target_link_libraries(test-player ${LD_PATH}/SDL2main)
# else ()
#     target_link_libraries(test-player ${LD_PATH}/libSDL2.so)
# endif ()
