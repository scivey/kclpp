#!/bin/bash

. $(dirname ${BASH_SOURCE[0]})/common.sh

pushd ${KCLPP_BINS_DIR}/dynamodb

DYNAMO_PORT="8093"
# DYNAMO_PORT="80"
java -Djava.library.path=./DynamoDBLocal_lib -jar DynamoDBLocal.jar -port ${DYNAMO_PORT} -inMemory -sharedDb

popd
