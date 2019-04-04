#!/bin/bash
#
# Top-level script for running unit tests.
# for manual testing run
# . install_boost.sh
# ./unit_tests.sh

ci_dir=$(dirname $BASH_SOURCE)
cd $ci_dir

. test_setup.sh

cd $testDir/unit || die "Hey where's the ./unit directory?"

# clear out target directory
[ ! -e obj ] || rm -rf obj

target_file=obj/runner

make all > build.log || die "Problem building unit tests. Please see build.log"

[ -f "$target_file" ] || die "Couldn't find the unit test executable"

: ${TRAVIS_BUILD_NUMBER:="0"}

# -r junit - use junit reporting
#$target_file -r junit -n "build_${TRAVIS_BUILD_NUMBER}" > obj/TEST-${TRAVIS_BUILD_NUMBER}.xml

make run > obj/TEST-${TRAVIS_BUILD_NUMBER}.xml

if [ "$?" == "0" ]; then
    echo Yay! Unit tests PASSED!    
else
    echo Bummer. Unit tests FAILED.
    exit 1
fi

# build test report
cd obj || die "cannot find obj dir"

# The test report was just for a demo. Need to find a better solution for
# production.
# cp ../../../../ci/unitth/* .
# java -jar unitth.jar . > unitth.log

set -x -e

# Run Communication Catch Tests
cd $ci_dir/../communication/tests/catch
make test PARTICLE_DEVELOP=1

# Run CMake-based unit tests
cd $unit_test_dir
rm -rf .build
mkdir .build && cd .build
cmake ..
make && make test


