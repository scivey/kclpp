cmake_minimum_required(VERSION 2.8)

project(kclpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 -Wall -O0 -g")

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
    ${SRC_ROOT}/kclpp/**/**/*.cpp
    ${SRC_ROOT}/kclpp/**/**/**/*.cpp

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



FILE(GLOB KCLPP_TEST_SUPPORT_SRC
    ${SRC_ROOT}/kclpp_test_support/*.cpp
    ${SRC_ROOT}/kclpp_test_support/**/*.cpp
)
add_library(kclpp_test_support ${KCLPP_TEST_SUPPORT_SRC})
add_dependencies(kclpp_test_support kclpp)
target_link_libraries(kclpp_test_support kclpp)


FILE(GLOB KCLPP_UNIT_TEST_SRC
    ${SRC_ROOT}/test/unit/*.cpp
    ${SRC_ROOT}/test/unit/**/*.cpp
)
add_executable(unit_test_runner
    ${KCLPP_UNIT_TEST_SRC}
    ${SRC_ROOT}/run_tests.cpp
)
add_dependencies(unit_test_runner kclpp kclpp_test_support)
target_link_libraries(unit_test_runner
    kclpp
    kclpp_test_support
    gmock
    ${COMMON_LIBS}
)


FILE(GLOB KCLPP_FUNCTIONAL_TEST_SRC
    ${SRC_ROOT}/test/functional/*.cpp
    ${SRC_ROOT}/test/functional/**/*.cpp
)
add_executable(func_test_runner
    ${KCLPP_FUNCTIONAL_TEST_SRC}
    ${SRC_ROOT}/run_api_tests.cpp
)
add_dependencies(func_test_runner kclpp kclpp_test_support)
target_link_libraries(func_test_runner
    kclpp
    kclpp_test_support
    gmock
    ${COMMON_LIBS}
)

