cmake_minimum_required(VERSION 2.8)
project(TcpNetwork)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
if (MSVC)
	# Exception handler model.
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DWIN32)
else (MSVC)
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif (MSVC)

# Source files.
set(SOURCES_TCP_NETWORK
	src/net/client.h
	src/net/local.cpp
	src/net/local.h
	src/net/network.cpp
	src/net/network.h
	src/net/packet.h
	src/net/remote.cpp
	src/net/remote.h
	src/net/server.cpp
	src/net/server.h
)

set(SOURCES_TCP_NETWORK_TEST
	srcTest/main.cpp
)
# End of source files.

find_package(SDL2 REQUIRED)
find_package(SDL2_net REQUIRED)

include_directories(
	${SDL2_INCLUDE_DIRS}
	${SDL2_NET_INCLUDE_DIRS}
)

add_library(TcpNetwork ${SOURCES_TCP_NETWORK})

include_directories(src)
add_executable(TcpNetworkTest ${SOURCES_TCP_NETWORK_TEST})

if (MSVC)
	set_target_properties(TcpNetworkTest PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:msvcrt")
endif (MSVC)

target_link_libraries(TcpNetworkTest
	TcpNetwork
	${SDL2_LIBRARIES}
	${SDL2_NET_LIBRARIES}
)
