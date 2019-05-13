#!/bin/bash

function display_help ()
{
    echo "\
usage: release-tests.sh [--dryrun] [--help]
                        [--filename=<test_parameter_file.json>]
                        --output-directory=<binary_output_directory>
                        --platform=<argon|asom|boron|bsom...
                        |core|electron|p1|photon|xenon|xsom>
                        --version=<semver_version_string>

Generate the testing binaries belonging to a given platform.

  -d, --dry-run           Print the compilation commands for a given
                            binary (as opposed to executing).
  -f, --filename          The file path and name of the desired
                            parameter file. If none is supplied, then
                            the file '<particle-iot/device-os>/user/tests..
                            /release-tests.json' will be used as default.
  -h, --help              Display this help and exit.
  -o, --output-directory  Specify the root output directory where the
                            folder hierarchy for the resulting binaries
                            will be placed.
  -p, --platform          Specify the desired platform.
  -v, --version           Specify the semantic version of the Device OS
                            for which you are building tests.
"
}

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

OPTIONS=df:ho:p:v:
LONGOPTS=dry-run,filename:,help,output-directory:,platform:,version:

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
        -h|--help)
            shift
            display_help
            exit 0
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

function valid_platform ()
{
    # Parse parameter(s)
    platform=$1

    # Validate platform (result of expression returned to caller)
    [ "$platform" = "argon" ] || [ "$platform" = "asom" ] || [ "$platform" = "boron" ] || [ "$platform" = "bsom" ] || [ "$platform" = "core" ] || [ "$platform" = "electron" ] || [ "$platform" = "p1" ] || [ "$platform" = "photon" ] || [ "$platform" = "xenon" ] || [ "$platform" = "xsom" ]
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
    "core")
        PLATFORM_ID="0"
        ;;
    "photon")
        PLATFORM_ID="6"
        ;;
    "p1")
        PLATFORM_ID="8"
        ;;
    "electron")
        PLATFORM_ID="10"
        ;;
    "argon")
        PLATFORM_ID="12"
        ;;
    "boron")
        PLATFORM_ID="13"
        ;;
    "xenon")
        PLATFORM_ID="14"
        ;;
    "asom")
        PLATFORM_ID="22"
        ;;
    "bsom")
        PLATFORM_ID="23"
        ;;
    "xsom")
        PLATFORM_ID="24"
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
    function append_metadata_seperator () {
        if [ $METADATA = false ]; then
            METADATA=true
            QUALIFIED_FILENAME+="+"
        else
            QUALIFIED_FILENAME+="."
        fi
    }
    function json () {
        local object_string=$1
        local member=$2
        echo $object_string | base64 --decode | jq -r $member
    }
    
    METADATA=false

    # Create tests directory
    TEST_DIRECTORY=$BINARY_DIRECTORY/$(json $test_object .path)/$(json $test_object .name)
    mkdir -p $TEST_DIRECTORY

    # Base strings
    MAKE_COMMAND="make -s all PLATFORM_ID=$PLATFORM_ID"
    QUALIFIED_FILENAME="${PLATFORM}-$(json $test_object .name)@${VERSION}"

    # Compose make command and file name
    if [ $(json $test_object .compile_lto) = true ]; then
        MAKE_COMMAND+=" COMPILE_LTO=y"
        append_metadata_seperator
        QUALIFIED_FILENAME+="lto"
    else
        MAKE_COMMAND+=" COMPILE_LTO=n"
    fi
    if [ $(json $test_object .debug_build) = true ]; then
        MAKE_COMMAND+=" DEBUG_BUILD=y"
        append_metadata_seperator
        QUALIFIED_FILENAME+="debug"
    else
        MAKE_COMMAND+=" DEBUG_BUILD=n"
    fi

    # Support Core
    if [ "$PLATFORM" == "core" ]; then
        MAKE_COMMAND+=" MODULAR=n"
    else
        MAKE_COMMAND+=" MODULAR=y"
    fi

    MAKE_COMMAND+=" USE_SWD_JTAG=n USE_SWD=n"
    
    # Append test commands and metadata
    MAKE_COMMAND+=" TEST=$(json $test_object .path)/$(json $test_object .name)"
    if [ $(json $test_object .use_threading) = true ]; then
        MAKE_COMMAND+=" USE_THREADING=y"
        append_metadata_seperator
        QUALIFIED_FILENAME+="multithreaded"
    else
        MAKE_COMMAND+=" USE_THREADING=n"
        append_metadata_seperator
        QUALIFIED_FILENAME+="singlethreaded"
    fi

    # Append user supplied arguments to commands and metadata
    USER_ARGS=$(json $test_object .user_args)
    if [ "$USER_ARGS" != "null" ]; then
        for user_arg in $(echo $USER_ARGS | jq '. | to_entries[]' --compact-output -r); do
            MAKE_COMMAND+=" $(echo $user_arg | jq -r '.key')=$(echo $user_arg | jq -r '.value')"
            append_metadata_seperator
            QUALIFIED_FILENAME+="$(echo $user_arg | jq -r '.value')"
        done
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
