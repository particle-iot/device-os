#!/bin/bash
# $1 handler script filename
# $2 test platforms root

# fail on first error
set -e

testroot=$2
ci_dir=$(dirname $0)
. $ci_dir/test_setup.sh

if [[ -z "$testroot" ]]; then
  testroot="$testDir"
fi

handler=$1
[ "$handler" ] || die "Usage: $0 [test-handler-script]"

# Enumerate a directory. The directory to enumerate is the last argument
# Invokes the arguments 1..N-1, replacing the last arg with the path of the 
# enumerated directory. (This allows nested funciton calls.)
function enum_dirs() {
  local base="${!#}" 
  local args=${@:1:$(($#-1))}
  local path 
  #echo "Enumerating directory $base , with $args" 
  for path in $base/*; do
    [ -d "$path" ] || continue
    #echo "running $args $path" 
    $args $path 
    [[ $? ]] || die "Problem in directory $path"
  done
}

function excluding() {
  local exclude=$1
  shift
  local args=$@
  local path="${!#}" 
  local dir=$(basename "$path") 
  if [[ ! "$dir" =~ $exclude ]]; then
     $args
  fi
}

function enum_platforms() {
  enum_dirs excluding "libraries|unit" $@ 
}

function enum_suites() {
  enum_dirs $@
}

function handle_suite() {
  local path=$1
  local suite=$(basename "$path") 
  local platform=$(basename $(dirname "$path"))  
  $handler "$path" "$platform" "$suite" 
}

export testFailures=$log_dir/test_failures
[[ ! -f $testFailures ]] || rm $testFailures 

# the main iteration over directories
enum_platforms enum_suites handle_suite $testroot
[[ "$?" ]] || die "Computer says 'no'."


if [ ! -f $testFailures ]; then
echo All tests SUCCEEDED. Nice work!
else 
echo There were test FAILURES. Please see the logs for details.
exit 1
fi

