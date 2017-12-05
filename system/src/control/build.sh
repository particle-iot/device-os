#!/bin/bash

# Note for macOS users:
#
# 1. The following dependencies are required to build the protocol files using the nanopb plugin:
#
# brew install protobuf python
# pip2 install protobuf
#
# 2. Make sure your system Python can find Python modules installed via Homebrew:
#
# mkdir -p ~/Library/Python/2.7/lib/python/site-packages
# echo 'import site; site.addsitedir("/usr/local/lib/python2.7/site-packages")' >> ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

NANOPB_PATH="${DIR}/../../../services/nanopb"
PROTOC_NANOPB_PLUGIN="${NANOPB_PATH}/generator/protoc-gen-nanopb"
PROTOC_INCLUDE_PATH="-I${DIR} -I${NANOPB_PATH}/generator -I${NANOPB_PATH}/generator/proto"

gen_proto() {
  protoc ${PROTOC_INCLUDE_PATH} --plugin=protoc-gen-nanopb=${PROTOC_NANOPB_PLUGIN} --nanopb_out=${DIR} "$1"
}

gen_proto "${DIR}/common.proto"
gen_proto "${DIR}/control.proto"
gen_proto "${DIR}/config.proto"
