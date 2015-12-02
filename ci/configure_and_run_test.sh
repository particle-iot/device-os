#!/bin/bash

# assumes test suite loaded and ready to rumble
# configures tests (sets included/excluded tests)
# starts the test suite
# fetches the log and test result

basedir=$1
platform=$2
suite=$3

ci_dir=$(dirname $0)
. ${ci_dir}/test_setup.sh 

# if test include file exists, parse to function calls and execute these
testInclude=${basedir}/test.include
[[ ! -f $testInclude ]] || 
(echo "applying test.include flags" && cat $testInclude | parseFlags | executeScript)

# tell the core to start the tests
echo Starting test suite $platform/$suite
startTests || die "Unable to start test suite"

# fetch the test log concurrently as tests run
echo Reading test log
testLogFile=${log_dir}/test_${platform}_${suite}.log
readTestLog > $testLogFile || die "Unable to read test log"

echo Verifying test complete
waitForState complete 60 || die "Test suite not complete"

testResultFile=${log_dir}/test_${platform}_${suite}_result.txt
readTestResult > $testResultFile || die "Unable to read test result"

# flag test failures file if the test failed.
( 
. $testResultFile
if [ "$failed" != "0" ]; then
  touch $testFailures
  echo "Yikes! Test $platform/$suite has FAILED."
  echo "Test log follows:"
  cat $testLogFile
  cat $testResultFile
else
  echo "Yey! Test $platform/$suite has passed!"
fi
)




