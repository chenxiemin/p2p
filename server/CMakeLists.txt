cmake_minimum_required (VERSION 2.6)

include_directories("../common")

# source files
FILE (GLOB SRC ./*.cpp ../common/*.cpp ../common/*.h)
add_executable (p2pserver ${SRC})

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
#     
#     target_link_libraries(test-player ${LD_PATH}/SDL2main)
# else ()
#     target_link_libraries(test-player ${LD_PATH}/libSDL2.so)

