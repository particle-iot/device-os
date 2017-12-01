#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

PROTO_FILES="${DIR}/*.proto"
NANOPB_PATH="${DIR}/../../../services/nanopb"
PROTOC_NANOPB_PLUGIN="${NANOPB_PATH}/generator/protoc-gen-nanopb"
PROTOC_INCLUDE_PATH="-I${DIR} -I${NANOPB_PATH}/generator -I${NANOPB_PATH}/generator/proto"

exec protoc ${PROTOC_INCLUDE_PATH} --plugin=protoc-gen-nanopb=${PROTOC_NANOPB_PLUGIN} --nanopb_out=${DIR} ${PROTO_FILES}
