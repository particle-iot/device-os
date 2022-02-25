#!/bin/bash
#
# Top-level script for running unit tests.
# for manual testing run
# . install_boost.sh
# ./unit_tests.sh

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
make all test coverage

cmake_unit_tests=$?

if [[ ${cmake_unit_tests} -eq 0 ]]; then
    echo Yay! Unit tests PASSED!
else
    echo Bummer. Unit tests FAILED.
    exit 1
fi
