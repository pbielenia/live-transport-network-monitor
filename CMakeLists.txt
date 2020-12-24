cmake_minimum_required(VERSION 3.17)

project(network-monitor)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
	set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
add_executable(network-monitor ${SOURCES})

