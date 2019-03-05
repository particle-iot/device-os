#!/bin/bash

# provides the script interface for communicating with a core running
# a test suite

export ci_dir=$(cd $(dirname $BASH_SOURCE) && pwd)

. $ci_dir/functions.sh

export build=$ci_dir/../build
export main=$ci_dir/../main
export user=$ci_dir/../user
export target_dir=$build/target
export target=core-firmware.bin
export events=$ci_dir/events.log
export target_file=$target_dir/main/platform-0/tests/$platform/$suite/main.bin
export testDir=$user/tests

# Path to CMake-based unit tests
export unit_test_dir=$ci_dir/../test/unit_tests

# directory for the test reports
export log_dir=${target_dir}/test-reports
mkdir -p ${log_dir}


# background task to pull down events from the core and write to file
function start_listening() {
  particle subscribe state $core_name > $events &
}

# terminate the most recently started background process
function stop_listening() {
  kill $!
}

# Converts from a state number to a state name
# $1 the state number to convert
function stateName() {
  local state=$1
  local result=""
  case $state in
    0)
      result="initializing"
      ;;
    1)
      result="waiting"
      ;;
    2)
      result="running"
      ;;
    3)
      result="complete"
  esac
  echo -n "$result"
}

# reads the current test suite execution state from the core
function readState() {
#  cat $events | grep "\"name\":\"state\",\"data\":\"$state\"" >> /dev/null
# I had originaly planned to monitor state by parsing the event logs, but this
# requires a background process. Polling the "state" variable is easier.
  local state=$(readVar state)
  stateName $state
}

# test if the current test has reached a given state
# $1 the state to look for
function testState() {
  local state=$1
  local currentState=$(readState)
  [ "$currentState" == "$state" ]
}

# waits for the test harness to reach a given state with timeout
# $1 the state name to wait for
# $2 the timeout to wait for
function waitForState() {
  local state=$1
  local timeout=$2
  # save success state
  echo "1" > success
  # This little bit of convolved script is to allow CTRL-C to interrupt
  # the timeout. The timeout command (a sub-shell) is ran as a background process
  # which we wait for. The interrupt trap terminates this process.
  # Without this, it's not possible to interrupt the timeout.
  (timeout $timeout bash << EOT
  source $ci_dir/test_setup.sh
  while ! testState $state; do
    sleep 5 || die
  done
  echo "0" > success  # flag success
EOT
) &
  trap 'kill -INT -$pid' INT
  pid=$!
  wait $pid
  return $(cat success)
}

function startTests() {
  [ "$(sendCommand start)"=="0" ]
}

# tests that match the given regex are flagged as included
# $1 the regex to include
function includeTests() {
  echo "include tests matching $1"
  [[ $(sendCommand "include=$1")=="0" ]]
}

function excludeTests() {
  echo "exclude tests matching $1"
  [[ $(sendCommand "exclude=$1")=="0" ]]
}

# sends a command to the test harness
# $1 the command string to send
function sendCommand() {
  r=$(particle function call $core_name cmd $1)
  echo $r
}

function parseFlags() {
  while read line; do
    local cmd="${line:0:1}"
    local regex="${line:1}"
    case "$cmd" in
    "+")
      echo "includeTests $regex"
      ;;
    "-")
      echo "excludeTests $regex"
      ;;
    esac
  done
}

# reads the log from the test suite. This is done by calling the
# command "log" to populate a variable with a segement of the log,
# reading the varaible, and then repeating the process until
# the log command returns -1, indicating end of log.
function readTestLog() {
  local len
  local val
  while len=$(sendCommand log) && [ -n "$len" ] && [ "$len" -gt "-1" ] && val=$(readVar log) ; do
    echo -n "${val:0:$len}"
  done
}

function readTestResult() {
  echoVar passed &&
  echoVar failed &&
  echoVar skipped &&
  echoVar count
}

function echoVar {
  echo $1=$(readVar $1)
}

function readVar() {
  particle variable get $core_name $1
}

function sparkFlash() {
    # flash the firmware - attempt to do this up to $1 times
    count=$1
    for ((n=1; n<=count; n++))
    do
        echo "OTA flashing firmware at $(date) - attempt $n"
        particle flash $2 $3 > otaflash
        [[ $? -eq 0 ]] && grep -q -v ECONN otaflash && break
    done

    [ $n -lt $count ]
    return $?
}
