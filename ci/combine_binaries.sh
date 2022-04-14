#!/bin/bash

set -e

SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")

RELEASE=$1
RELEASE_REF=$2
OUTPUT_DIR=$3

RELEASE_PUBLISH=$SCRIPT_DIR/release-publish.sh
OUTPUT_BINARIES_DIR="$OUTPUT_DIR/binaries-all/"
OUTPUT_RELEASE_DIR=$OUTPUT_DIR/release

find $RELEASE

mkdir -p $OUTPUT_BINARIES_DIR/${RELEASE_REF}
pushd $OUTPUT_BINARIES_DIR/${RELEASE_REF}
for z in $(find $RELEASE -name '*\+*.zip'|grep -v binaries); do
  unzip $z -d .
done
cp -r $RELEASE/*/$RELEASE_REF/* ./
popd

$RELEASE_PUBLISH --release-directory $OUTPUT_BINARIES_DIR

mkdir -p $OUTPUT_RELEASE_DIR
pushd $OUTPUT_BINARIES_DIR/${RELEASE_REF}
zip $OUTPUT_RELEASE_DIR/particle_device-os@${RELEASE_REF}.zip . --recurse-paths --quiet
popd
find $OUTPUT_BINARIES_DIR
cp -r $RELEASE/coverage $OUTPUT_RELEASE_DIR
find $OUTPUT_RELEASE_DIR

exit 0
