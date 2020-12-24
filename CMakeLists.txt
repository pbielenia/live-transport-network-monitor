cmake_minimum_required(VERSION 3.17)

project(network-monitor)
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
add_executable(network-monitor ${SOURCES})

