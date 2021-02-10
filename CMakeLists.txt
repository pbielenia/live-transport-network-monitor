cmake_minimum_required(VERSION 3.17)

project(network-monitor)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif()

find_package(Boost 1.74 REQUIRED COMPONENTS system unit_test_framework)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

enable_testing()

add_library(network-monitor STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/file-downloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport-network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-client.cpp
    )
target_compile_features(network-monitor
    PUBLIC
        cxx_std_20
    )

target_link_libraries(network-monitor
    PUBLIC
        Boost::Boost
        OpenSSL::OpenSSL
        nlohmann_json::nlohmann_json

    PRIVATE
        CURL::CURL
    )
target_include_directories(network-monitor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/network-monitor>
    )

set(TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/file-downloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/transport-network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp
    )
add_executable(network-monitor-tests ${TESTS_SOURCES})
target_compile_features(network-monitor-tests
    PRIVATE
        cxx_std_17
    )
target_compile_definitions(network-monitor-tests
    PRIVATE
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TESTS_NETWORK_LAYOUT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json"
        TESTS_NETWORK_LAYOUTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/network_layouts"
    )
target_link_libraries(network-monitor-tests
    PRIVATE
        network-monitor
        Boost::Boost
        OpenSSL::OpenSSL
        nlohmann_json::nlohmann_json
    )
add_test(
    NAME network-monitor-tests
    COMMAND $<TARGET_FILE:network-monitor-tests>
    )
set_tests_properties(network-monitor-tests PROPERTIES
    PASS_REGULAR_EXPRESSION ".*No errors detected"
    )
