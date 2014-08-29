#!/bin/bash
#
# Top-level script for running integration tests.
# Enumerates all tests, passing each test to the handle_test script.
ci_dir=$(dirname $BASH_SOURCE)
cd $ci_dir
./enumerate_tests.sh ./handle_test.sh
