
GREEN="\033[32m"
RED="\033[31m"
NO_COLOR="\033[0m"

function checkFailures {
  if [[ $HAS_FAILURES == 1 ]] || [[ -f '.has_failures' ]]; then
    echo "Ruh roh. Failed tests."
    return 1
  fi
  return 0
}

function testcase {
  if [ $? -eq 0 ]; then
    echo -e "$GREEN ✓ SUCCESS $NO_COLOR"
    return 0
	else
    echo -e "$RED ✗ FAILED $NO_COLOR"
    export HAS_FAILURES=1
    touch '.has_failures'
    return 1
  fi
}


function contains {
  loop=($1)
  for i in "${loop[@]}"
  do
    if [[ "$i" == "$2" ]] ; then
      return 0
    fi
  done
  return 1
}


function emptyOrContains {
  [[ "" == "$1" ]] || contains "$1" "$2"
}

function platform {
  emptyOrContains "${BUILD_PLATFORM[*]}" "$1"
}

function filterPlatform {
  eval 'array=${'${1}'[@]}'
  eval "array=( $array )"
  result=""
  for i in "${array[@]}"; do
    if platform $i; then
      result="$result $i"
    fi
  done
  # remove leading space
  eval "$1=( ${result} )"
}

function die {
  [[ -z "$1" ]] || echo "Error: $1"
  exit 1
}

# executes script in this shell passed as stdin
function executeScript {
  while read line; do
    eval $line
  done
}

function checkDefined() {   
  [ "${!1}" ] || die "\$$1 variable is awol."
}
