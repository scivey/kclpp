#!/bin/bash

. $(dirname ${BASH_SOURCE[0]})/common.sh

pushd ${KCLPP_SUPPORT_DIR}

if [[ ! -d node_modules ]]; then
    npm install
fi

./node_modules/.bin/kinesalite \
    --port 8094 \
    --createStreamMs 5 \
    --deleteStreamMs 5 \
    --updateStreamMs 5

popd
