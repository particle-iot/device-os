#!/bin/bash

## Building the combined Image
# set WICED_SDK to point to the directory containing the photon-wiced repo, with the
# `feature/combined-fw` branch checked out
# (optional) set DEVICE_OS_DIR to point to the directory containing the device-os repo with the
# necessary release tag checked out
# Run this script, specifying the target PLATFORM_ID, e.g.: ./make_combined.sh PLATFORM_ID=6
# This will build the artefacts to $(DEVICE_OS_DIR)/build/releases/release-$(DEVICE_OS_VERSION)-p$(PLATFORM_ID)
# e.g. $(DEVICE_OS_DIR)/build/releases/release-1.2.1-p6 for Photon

# ensure bytes are treated as bytes by tr or /xFF is output as a UTF-8 byte pair
export LC_CTYPE=C

export PWD=$(cd $(dirname $BASH_SOURCE) && pwd)

DEVICE_OS_DIR="${DEVICE_OS_DIR:-${PWD}/../../../}"
if [ -z "${WICED_SDK}" ]; then
    echo "WICED_SDK needs to be set"
    exit 1
fi

make -f wiced_test_mfg_combined.mk FIRMWARE="${DEVICE_OS_DIR}" WICED_SDK="${WICED_SDK}" $*
