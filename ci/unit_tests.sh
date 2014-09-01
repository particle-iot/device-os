#!/bin/bash
#
# Top-level script for running unit tests.
ci_dir=$(dirname $BASH_SOURCE)
cd $ci_dir

. test_setup.sh

cd ../unit || die "Hey where's the ./unit directory?"

target_file=obj/runner

make all > build.log || die "Problem building unit tests"

[ -f "$target_file" ] || die "Couldn't find the unit test executable"

$target_file > test.log

if [ "$?" == "0" ]; then
    echo Yay! Unit tests PASSED!    
else
    echo Bummer. Unit tests FAILED.
    exit 1
fi






