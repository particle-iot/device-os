#!/bin/bash

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

OPTIONS=df:o:p:v:
LONGOPTS=dry-run,filename:,output-directory:,platform:,version:

# -use ! and PIPESTATUS to get exit code with errexit set
# -temporarily store output to be able to check for errors
# -activate quoting/enhanced mode (e.g. by writing out “--options”)
# -pass arguments only via   -- "$@"   to separate them correctly
! PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTS --name "$0" -- "$@")
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    # e.g. return value is 1
    #  then getopt has complained about wrong arguments to stdout
    exit 2
fi

# Read getopt’s output this way to handle the quoting right:
eval set -- "$PARSED"

# Set default(s)
DRY_RUN=false
PARAMETER_FILE="../user/tests/release-tests.json"
OUTPUT_DIRECTORY=""
PLATFORM=""
VERSION=""

# Parse parameter(s)
while true; do
    case "$1" in
        -d|--dry-run)
            DRY_RUN=true
            shift 1
            ;;
        -f|--filename)
            PARAMETER_FILE="$2"
            shift 2
            ;;
        -o|--output-directory)
            OUTPUT_DIRECTORY="$2"
            shift 2
            ;;
        -p|--platform)
            PLATFORM="$2"
            shift 2
            ;;
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Encountered error while parsing arguments!"
            exit 3
            ;;
    esac
done

function valid_platform()
{
    # Parse parameter(s)
    platform=$1

    # Validate platform (result of expression returned to caller)
    [ "$platform" = "core" ] || [ "$platform" = "electron" ] || [ "$platform" = "p1" ] || [ "$platform" = "photon" ]
}

# Handle invalid arguments
if [ $# -ne 0 ]; then
    echo "$0: Unknown argument \"$1\" supplied!"
    exit 4
elif [ -z $OUTPUT_DIRECTORY ]; then
    echo "--output-directory argument must be specified!"
    exit 5
elif [ -z $PLATFORM ]; then
    echo "--platform argument must be specified!"
    exit 6
elif !(valid_platform $PLATFORM); then
    echo "Invalid platform specified!"
    exit 7
elif [ -z $VERSION ]; then
    echo "--version argument must be specified!"
    exit 8
fi

# Infer platform id
case "$PLATFORM" in
    core)
        PLATFORM_ID="0"
        ;;
    photon)
        PLATFORM_ID="6"
        ;;
    p1)
        PLATFORM_ID="8"
        ;;
    electron)
        PLATFORM_ID="10"
        ;;
    *)
        echo "ERROR: No rules to release platform: \"$PLATFORM\"!"
        exit 9
        ;;
esac

# Eliminate relative paths
mkdir -p $OUTPUT_DIRECTORY
pushd $OUTPUT_DIRECTORY > /dev/null
ABSOLUTE_OUTPUT_DIRECTORY=$(pwd)
popd > /dev/null

TARGET_DIRECTORY=../build/target
mkdir -p $TARGET_DIRECTORY
pushd $TARGET_DIRECTORY > /dev/null
ABSOLUTE_TARGET_DIRECTORY=$(pwd)
popd > /dev/null

# Create binary output directories
QUALIFIED_OUTPUT_DIRECTORY=$ABSOLUTE_OUTPUT_DIRECTORY/$VERSION/$PLATFORM
BINARY_DIRECTORY=$QUALIFIED_OUTPUT_DIRECTORY/tests

# Move to `main` directory for building tests
cd ../main

for test_object in $(jq '.platforms[] | select(.platform == "'${PLATFORM}'") | .tests[] | select(.enabled == true) | @base64' --compact-output -r ${PARAMETER_FILE}); do
    function json () {
        local object_string=$1
        local member=$2
        echo $object_string | base64 --decode | jq -r $member
    }

    # Create tests directory
    TEST_DIRECTORY=$BINARY_DIRECTORY/$(json $test_object .path)/$(json $test_object .name)
    mkdir -p $TEST_DIRECTORY

    # Base strings
    MAKE_COMMAND="make -s all TEST=$(json $test_object .path)/$(json $test_object .name) PLATFORM=$PLATFORM"
    QUALIFIED_FILENAME="$(json $test_object .name)@${VERSION}+${PLATFORM}"

    # Compose file name
    if [ $(json $test_object .compile_lto) = true ]; then
        MAKE_COMMAND+=" COMPILE_LTO=y"
        QUALIFIED_FILENAME+=".lto"
    else
        MAKE_COMMAND+=" COMPILE_LTO=n"
        QUALIFIED_FILENAME+=".m"
    fi
    if [ $(json $test_object .debug_build) = true ]; then
        MAKE_COMMAND+=" DEBUG_BUILD=y"
        QUALIFIED_FILENAME+=".debug"
    else
        MAKE_COMMAND+=" DEBUG_BUILD=n"
        QUALIFIED_FILENAME+=".ndebug"
    fi
    
    # Append test metadata
    if [ $(json $test_object .use_threading) = true ]; then
        MAKE_COMMAND+=" USE_THREADING=y"
        QUALIFIED_FILENAME+=".multithreaded"
    else
        MAKE_COMMAND+=" USE_THREADING=n"
        QUALIFIED_FILENAME+=".singlethreaded"
    fi

    # Support Core
    if [ "$PLATFORM" == "core" ]; then
        BUILD_DIRECTORY=$ABSOLUTE_TARGET_DIRECTORY/main/platform-$PLATFORM_ID-lto
    else
        BUILD_DIRECTORY=$ABSOLUTE_TARGET_DIRECTORY/user-part/platform-$PLATFORM_ID-m
    fi

    # Clear build directory
    echo $MAKE_COMMAND
    if [ $DRY_RUN = false ]; then
        rm -rf $BUILD_DIRECTORY/*
        eval $MAKE_COMMAND
        mv ${BUILD_DIRECTORY}/*.bin ${TEST_DIRECTORY}/${QUALIFIED_FILENAME}.bin
    fi
done
