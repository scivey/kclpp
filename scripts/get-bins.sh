#!/bin/bash

. $(dirname ${BASH_SOURCE[0]})/common.sh


function get-dynamodb() {
    local target_dir=${KCLPP_BINS_DIR}/dynamodb
    mkdir -p ${target_dir}
    pushd ${target_dir}
    local tarball_url="http://dynamodb-local.s3-website-us-west-2.amazonaws.com/dynamodb_local_latest.tar.gz"
    local outname="dynamodb_local.tar.gz"
    if [[ ! -f DynamoDBLocal.jar ]]; then
        if [[ ! -f $outname ]]; then
            wget -O $outname $tarball_url
        fi
        tar -xaf $outname
    fi
    popd
}

function get-kinesalite() {
    pushd ${KCLPP_SUPPORT_DIR}
    if [[ ! -f node_modules/.bin/kinesalite ]]; then
        npm install
    fi
    popd
}


function get-bins() {
    mkdir -p ${KCLPP_BINS_DIR}
    get-dynamodb
    get-kinesalite
}

get-bins
