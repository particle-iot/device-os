#!/bin/bash
# handles a test suite: makes, runs and collects results
# $1 - the directory containing the test
# $2 - the test platform
# $3 - the test suite
testdir=$1
platform=$2
suite=$3

# initialze environment
ci_dir=$(dirname $0)
. $ci_dir/test_setup.sh
. $ci_dir/functions.sh

# bring in info on platform -> cores
. $ci_dir/cores

export core_name=${name[$platform]}

if [ -z "$core_name" ]; then
  echo "No core defined for platform $platform. Skipping test $platform/$suite."
  exit 0
fi

# make the firmware 
echo Building test suite in "$testdir"
$ci_dir/make_test.sh $platform $suite || die 

sparkFlash 5 $core_name $target_file || { echo "Unable to OTA flash test suite"; die; }


echo "Waiting for core to reboot"
# todo - verify test suite build time or fix particle-cli return codes
# give enough time for the core to go into OTA mode
sleep 10 || die

echo "Waiting for test suite to party... $(date)"
waitForState waiting 300 || die "Timeout waiting for test suite. $(date)"

# do it
$ci_dir/configure_and_run_test.sh $1 $2 $3 
result=$?
echo Test $platform/$suite complete. 
exit "$result"
