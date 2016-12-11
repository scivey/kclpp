#!/bin/bash

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

KCLPP_SCRIPTS_DIR=$(dirname ${BASH_SOURCE[0]})
pushd ${KCLPP_SCRIPTS_DIR}/..
export KCLPP_ROOT_DIR=$(pwd)
export KCLPP_SCRIPTS_DIR=${KCLPP_SCRIPTS_DIR}/scripts
export KCLPP_BINS_DIR=${KCLPP_ROOT_DIR}/.bin
export KCLPP_SUPPORT_DIR=${KCLPP_ROOT_DIR}/support
popd

