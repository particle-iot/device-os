#!/bin/bash

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

# Create a virtual environment
python3 -m venv "$PROTO_DEFS_DIR/.venv"
source "$PROTO_DEFS_DIR/.venv/bin/activate"

# Install dependencies
pip3 install protobuf

# Compile control request definitions
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

# Compile cloud protocol definitions
gen_proto "${SHARED_DIR}/cloud/ledger.proto"
gen_proto "${SHARED_DIR}/cloud/cloud.proto"
gen_proto "${SHARED_DIR}/cloud/describe.proto"
gen_proto "${SHARED_DIR}/cloud/ledger.proto"

# Compile internal definitions
gen_proto "${INTERNAL_DIR}/network_config.proto"
gen_proto "${INTERNAL_DIR}/ledger.proto"
