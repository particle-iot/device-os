#!/bin/bash

# ensure we are in the root
cd "$( dirname "${BASH_SOURCE[0]}" )/.."

# Just in case remove .has_failures from root
rm -f "${PWD}/.has_failures"

export DEVICE_OS_ROOT=$PWD

if [ -n "$CI_BUILD_RELEASE" ]; then
    source ./ci/functions.sh
    source ./.buildpackrc
    echo "BUILD_PLATFORM=${BUILD_PLATFORM}"
    filterPlatform PRERELEASE_PLATFORMS
    filterPlatform RELEASE_PLATFORMS
    CI_RELEASE_PLATFORMS_ARRAY=( "${PRERELEASE_PLATFORMS[@]}" "${RELEASE_PLATFORMS[@]}" )
    export CI_RELEASE_PLATFORMS="${CI_RELEASE_PLATFORMS_ARRAY[@]}"
    echo "CI_RELEASE_PLATFORMS=${CI_RELEASE_PLATFORMS}"
    BUILD_PLATFORM_FILTERED=""
    BUILD_PLATFORM_ORIGINAL=($BUILD_PLATFORM)
    for p in "${BUILD_PLATFORM_ORIGINAL[@]}"; do
        if ! contains "${CI_RELEASE_PLATFORMS[*]}" "${p}"; then
            BUILD_PLATFORM_FILTERED="${BUILD_PLATFORM_FILTERED} $p"
        fi
    done
    export BUILD_PLATFORM="${BUILD_PLATFORM_FILTERED}"
    echo "BUILD_PLATFORM=${BUILD_PLATFORM}"
    ./ci/ci_release.sh
    RET=$?
    echo "ci/ci_release.sh ret=${RET}"
    cd /firmware/build
    checkFailures
    RET=$?
    if [ $RET -ne 0 ]; then
        exit $RET
    fi
    export BUILD_PLATFORM="${BUILD_PLATFORM_ORIGINAL[@]}"
    echo "BUILD_PLATFORM=$BUILD_PLATFORM"
fi

cd $DEVICE_OS_ROOT
$DEVICE_OS_ROOT/ci/enumerate_build_matrix.sh
