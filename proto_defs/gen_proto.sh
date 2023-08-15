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

set -e

DEVICE_OS_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PROTO_DEFS_DIR="$DEVICE_OS_DIR/proto_defs"
SHARED_DIR="$PROTO_DEFS_DIR/shared"
INTERNAL_DIR="$PROTO_DEFS_DIR/internal"
DEST_DIR="$PROTO_DEFS_DIR/src"

NANOPB_DIR="$DEVICE_OS_DIR/third_party/nanopb/nanopb"
NANOPB_PLUGIN_DIR="$NANOPB_DIR/generator/protoc-gen-nanopb"

gen_proto() {
  echo "Compiling $1"
  protoc -I"$NANOPB_DIR/generator/proto" \
         -I"$SHARED_DIR" \
         -I"$(dirname "$1")" \
         --plugin="protoc-gen-nanopb=$NANOPB_PLUGIN_DIR" \
         --nanopb_out="${DEST_DIR}" "$1"
}

# Control requests
gen_proto "${SHARED_DIR}/control/extensions.proto"
gen_proto "${SHARED_DIR}/control/common.proto"
gen_proto "${SHARED_DIR}/control/config.proto"
gen_proto "${SHARED_DIR}/control/wifi.proto"
gen_proto "${SHARED_DIR}/control/wifi_new.proto"
gen_proto "${SHARED_DIR}/control/cellular.proto"
gen_proto "${SHARED_DIR}/control/network.proto"
gen_proto "${SHARED_DIR}/control/network_old.proto"
gen_proto "${SHARED_DIR}/control/storage.proto"
gen_proto "${SHARED_DIR}/control/cloud.proto"

# Cloud protocol
gen_proto "${SHARED_DIR}/cloud/cloud.proto"
gen_proto "${SHARED_DIR}/cloud/describe.proto"

# Internal definitions
gen_proto "${INTERNAL_DIR}/network_config.proto"
