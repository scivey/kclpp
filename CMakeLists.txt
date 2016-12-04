cmake_minimum_required(VERSION 2.8)

project(kclpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -Wall -O0 -g")

set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(GTEST ${EXTERNAL}/googletest)

set(AWS_ROOT ${EXTERNAL}/aws-sdk-cpp)
set(AWS_BUILD ${AWS_ROOT}/build)

set(AWS_INCLUDES
    ${AWS_ROOT}/aws-cpp-sdk-core/include
    ${AWS_ROOT}/aws-cpp-sdk-kinesis/include
    ${AWS_ROOT}/aws-cpp-sdk-dynamodb/include
)

set(AWS_LINKS
    ${AWS_BUILD}/aws-cpp-sdk-core
    ${AWS_BUILD}/aws-cpp-sdk-kinesis
    ${AWS_BUILD}/aws-cpp-sdk-dynamodb
)

include_directories(
    ${GTEST}/googletest/include
    ${GTEST}/googlemock/include
    ${AWS_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/local/include
)

link_directories(
    ${GTEST}/build/googlemock
    ${AWS_LINKS}
    /usr/local/lib
)

set(COMMON_LIBS
    event
    glog
    aws-cpp-sdk-kinesis
    aws-cpp-sdk-dynamodb
    aws-cpp-sdk-core
    crypto
    curl
    pthread
    atomic
)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

FILE(GLOB KCLPP_LIB_SRC
    ${SRC_ROOT}/kclpp/*.cpp
    ${SRC_ROOT}/kclpp/**/*.cpp
)

add_library(kclpp
    ${KCLPP_LIB_SRC}
)
target_link_libraries(kclpp ${COMMON_LIBS})

add_executable(runner
    ${SRC_ROOT}/main.cpp
)

add_dependencies(runner kclpp)
target_link_libraries(runner kclpp ${COMMON_LIBS})


FILE(GLOB KCLPP_TEST_SRC
    ${SRC_ROOT}/test/*.cpp
    ${SRC_ROOT}/test/**/*.cpp
)
add_executable(test_runner
    ${KCLPP_TEST_SRC}
    ${SRC_ROOT}/run_tests.cpp
)
add_dependencies(test_runner kclpp)

target_link_libraries(test_runner
    kclpp
    gmock
    ${COMMON_LIBS}
)
