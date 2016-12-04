#!/bin/bash

ROOT=$(git rev-parse --show-toplevel)

EXT=${ROOT}/external

function cmake-build() {
    if [[ ! -d "build" ]]; then
        mkdir build
        pushd build
        cmake ../ && make -j8
        popd
    fi
}

GTEST_DIR=${EXT}/googletest

function build-gtest() {
    pushd ${GTEST_DIR}
    cmake-build
    popd
}

function clean-gtest() {
    pushd ${GTEST_DIR}
    rm -rf build
    popd
}


AWS_LIBS="dynamodb kinesis core"
AWS_SDK_DIR=${EXT}/aws-sdk-cpp

function clean-aws-sdk() {
    pushd ${AWS_SDK_DIR}
    rm -rf build
    popd
}

function build-aws-sdk() {
    pushd ${AWS_SDK_DIR}
    if [[ ! -d "build" ]]; then
        mkdir build
        pushd build
        cmake -DBUILD_SHARED_LIBS=OFF ../
        popd
    fi
    pushd build

    for lib in ${AWS_LIBS}; do
        if [[ ! -f "aws-cpp-sdk-${lib}/libaws-cpp-sdk-${lib}.a" ]]; then
            make aws-cpp-sdk-${lib} -j8
        fi
    done
    popd
    popd
}



case "$1" in
build)
    build-gtest
    build-aws-sdk
    ;;
clean)
    clean-gtest
    clean-aws-sdk
    ;;
*)
    echo "usage: ${BASH_SOURCE[0]} build|clean" >& 2
    exit 1
    ;;
esac
