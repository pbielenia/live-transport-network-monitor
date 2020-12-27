cmake_minimum_required(VERSION 3.17)

project(network-monitor)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif()
find_package(Boost 1.74 REQUIRED COMPONENTS system)

add_library(network-monitor STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-client.cpp
    )
target_compile_features(network-monitor
    PUBLIC
        cxx_std_14
    )
target_link_libraries(network-monitor
    PUBLIC
        Boost::Boost
    )
target_include_directories(network-monitor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/network-monitor>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

set(TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp
    )
add_executable(network-monitor-tests ${TESTS_SOURCES})
target_compile_features(network-monitor-tests
    PUBLIC
        cxx_std_14
    )
target_link_libraries(network-monitor-tests PUBLIC network-monitor)
