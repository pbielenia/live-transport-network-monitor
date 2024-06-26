cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(network-monitor)

# Defines
set(NETWORK_MONITOR_LIBRARY_NAME ${PROJECT_NAME})
set(NETWORK_MONITOR_TESTS_EXE_NAME ${PROJECT_NAME}-tests)

set(CMAKE_BUILD_TYPE Debug)

# Dependencies
if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()
find_package(Boost 1.80 REQUIRED COMPONENTS system unit_test_framework)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)

enable_testing()

# Library
add_library(${NETWORK_MONITOR_LIBRARY_NAME} STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/file-downloader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stomp-frame.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stomp-frame-builder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/transport-network.cpp"
)
target_compile_features(${NETWORK_MONITOR_LIBRARY_NAME}
    PRIVATE
        cxx_std_17
)
target_link_libraries(${NETWORK_MONITOR_LIBRARY_NAME}
    PUBLIC
        Boost::Boost
        OpenSSL::OpenSSL
        nlohmann_json::nlohmann_json
    PRIVATE
        CURL::libcurl
)
target_include_directories(${NETWORK_MONITOR_LIBRARY_NAME}
    PUBLIC
        inc
)

# Tests
add_executable(${NETWORK_MONITOR_TESTS_EXE_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/file-downloader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-frame.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-frame-builder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/transport-network.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client-mock.cpp"
)
target_compile_features(${NETWORK_MONITOR_LIBRARY_NAME}
    PRIVATE
        cxx_std_17
)
target_compile_definitions(${NETWORK_MONITOR_TESTS_EXE_NAME}
    PRIVATE
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TESTS_NETWORK_LAYOUT_JSON="${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json"
        TESTS_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/resources"
)
target_link_libraries(${NETWORK_MONITOR_TESTS_EXE_NAME}
    PRIVATE
        ${NETWORK_MONITOR_LIBRARY_NAME}
        Boost::Boost
        OpenSSL::OpenSSL
)

add_test(
    NAME ${NETWORK_MONITOR_TESTS_EXE_NAME}
    COMMAND $<TARGET_FILE:${NETWORK_MONITOR_TESTS_EXE_NAME}>
)
set_tests_properties(${NETWORK_MONITOR_TESTS_EXE_NAME} PROPERTIES
    PASS_REGULAR_EXPRESSION ".*No errors detected"
)
