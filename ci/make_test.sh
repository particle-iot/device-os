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
checkDefined main
checkDefined target_file

# where the build directory is. Presently the makefile must be
# run with that as the cwd
cd $main || die "couldn't change directory to '$main'"

# just for sanity test, remove existing firmware
[[ ! -f $target_file ]] || rm $target_file
  
make v=1 TEST=$platform/$suite || die 

[[ -f $target_file ]] || die "Expected target file '$target_file' to be produced by make."

 
