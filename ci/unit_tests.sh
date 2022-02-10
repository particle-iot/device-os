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

cd $testDir/unit || die "Hey where's the ./unit directory?"

# clear out target directory
[ ! -e obj ] || rm -rf obj

target_file=obj/runner

make all
make_unit_tests=$?

if [ -f "$target_file" ]; then

    : ${TRAVIS_BUILD_NUMBER:="0"}

    make run > obj/TEST-${TRAVIS_BUILD_NUMBER}.xml
    make_unit_tests=$?
fi

# Run CMake-based unit tests
cd $unit_test_dir
rm -rf .build/*
mkdir -p .build/
cd .build/
cmake $cmake_args ..
make all test coverage

cmake_unit_tests=$?

if [[ ${make_unit_tests} -eq 0 ]] && [[ ${cmake_unit_tests} -eq 0 ]]; then
    echo Yay! Unit tests PASSED!
else
    echo Bummer. Unit tests FAILED.
    exit 1
fi
