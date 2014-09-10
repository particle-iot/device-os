#!/bin/bash
#
# make the test binary as core-firmware.bin
platform=$1
suite=$2
ci_dir=$(dirname $0)
. $ci_dir/functions.sh


echo "Building test $suite for platform $platform"

# ensure build var is set
checkDefined build
checkDefined target_file
checkDefined target

# where the build directory is. Presently the makefile must be
# run with that as the cwd
cd $build || die "couldn't change directory to '$build'"

# just for sanity test, remove existing firmware
[[ ! -f $target_file ]] || rm $target_file
  
make TARGET=$target TEST=$platform/$suite > build_${platform}_${suite}.log || die 

[[ -f $target_file ]] || die "Expected target file '$target_file' to be produced by make."

 
