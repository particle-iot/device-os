#!/bin/bash

set -x

echo "NVM_DIR=$NVM_DIR"

[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"

npm install --unsafe-perm --silent --no-progress -g @particle/device-os-test-runner

export RELEASE_REF=$(cat /firmware/.git/ref)
export RELEASE_OUTPUT="$PWD/compiled-binaries/"

echo $CI_RELEASE_PLATFORMS

source /firmware/ci/functions.sh

pushd /firmware/build

for PLATFORM in $CI_RELEASE_PLATFORMS; do
    echo $PLATFORM
    VERSION="$RELEASE_REF" ./make_release.sh --platform=$PLATFORM --output-directory $RELEASE_OUTPUT
    testcase "0 PLATFORM=\"$PLATFORM\" ./make_release.sh"

    ls -laR $RELEASE_OUTPUT
    # Build test binaries
    export TEST_DIR=$RELEASE_OUTPUT/*/$PLATFORM/
    export TEST_DIR=$(echo ${TEST_DIR})/test
    mkdir -p $TEST_DIR
    echo "{\"api\": {\"token\": \"${DEVICE_OS_CI_API_TOKEN}\"}}" > /tmp/test.config.json
    pushd /firmware/user/tests/integration
    npm install
    popd
    ls -laR $TEST_DIR
    rm -rf /firmware/build/target
    device-os-test --device-os-dir=/firmware --target-dir=$TEST_DIR --binary-dir=$TEST_DIR --config-file=/tmp/test.config.json build $PLATFORM
    testcase "0 PLATFORM=\"$PLATFORM\" device-os-test"
    pushd /firmware/user/tests/integration
    find -L ./ -name '*.spec.js' -exec cp --parents {} $TEST_DIR/$PLATFORM \;
    cp package*.json $TEST_DIR/$PLATFORM/ || true
    popd
    ls -laR $RELEASE_OUTPUT
    ./release-publish.sh --release-directory $RELEASE_OUTPUT
    ls -laR $RELEASE_OUTPUT
    pushd $RELEASE_OUTPUT
    # rename "s/.zip/-$RELEASE_REF.zip/" */publish/*.zip
    popd
    find $RELEASE_OUTPUT
done

popd
