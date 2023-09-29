#!/bin/bash
#
# Top-level script for running unit tests.
#
# For manual testing from device-os/ run
# source ci/install_boost.sh
# source ci/build_boost.sh (once)
# ci/unit_tests.sh

set -x

if [ -n "$BOOST_ROOT" ]; then
    cmake_args="-DBoost_NO_SYSTEM_PATHS=TRUE"
fi

ci_dir=$(dirname $BASH_SOURCE)
cd $ci_dir

. test_setup.sh

# Run CMake-based unit tests
cd $unit_test_dir
rm -rf .build/*
mkdir -p .build/
cd .build/
cmake $cmake_args ..
make all test coverage CTEST_OUTPUT_ON_FAILURE=TRUE

cmake_unit_tests=$?

if [[ ${cmake_unit_tests} -eq 0 ]]; then
    echo Yay! Unit tests PASSED!
else
    echo Bummer. Unit tests FAILED.
    exit 1
fi
