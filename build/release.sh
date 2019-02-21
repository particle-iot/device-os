#!/bin/bash
set -o errexit -o pipefail -o noclobber -o nounset

VERSION="1.0.1"

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

OPTIONS=di:p:o:
LONGOPTS=debug,platform-id:,platform:,output-directory:

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
DEBUG=false
PLATFORM=""
PLATFORM_ID=""
OUTPUT_DIRECTORY="../build/releases"

# Parse parameter(s)
while true; do
    case "$1" in
        -d|--debug)
            DEBUG=true
            shift 1
            ;;
        -i|--platform-id)
            PLATFORM_ID="$2"
            shift 2
            ;;
        -p|--platform)
            PLATFORM="$2"
            shift 2
            ;;
        -o|--output-directory)
            OUTPUT_DIRECTORY="$2"
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

# Handle invalid arguments
if [ $# -ne 0 ]; then
    echo "$0: Unknown argument \"$1\" supplied!"
    exit 4
fi

function release_file()
{
    # Parse parameter(s)
    local name=$1
    local ext=$2
    local suffix=$3

    # Move file from build to release folder
    cp ../build/target/$name/platform-$PLATFORM_ID-$suffix/$name.$ext $BINARY_DIRECTORY/$VERSION+$PLATFORM.$name.$ext
}

function release_binary()
{
    # Parse parameter(s)
    local name=$1
    local suffix=${2:-m}

    # Move files into release folder
    release_file $name bin $suffix
    release_file $name elf $suffix
    release_file $name hex $suffix
    release_file $name lst $suffix
    release_file $name map $suffix
}

function release_file_core()
{
    # Parse parameter(s)
    local name=$1
    local ext=$2

    # Move file from build target to release folder
    cp $OUT_CORE/$name.$ext $BINARY_DIRECTORY/$VERSION+$PLATFORM.$name.$ext
}

function release_binary_core()
{
    # Parse parameter(s)
    local name=$1

    release_file_core $name bin
    release_file_core $name elf
    release_file_core $name hex
    release_file_core $name lst
    release_file_core $name map
}

function release_binary_module()
{
    # Parse parameter(s)
    local source_name=$1
    local target_name=$2

    # Move file from build target to release folder
    cp $OUT_MODULE/$source_name.bin $BINARY_DIRECTORY/$VERSION+$PLATFORM.$target_name.bin
}

# Align platform data (prefer name)
if [ -z $PLATFORM ] && [ -z $PLATFORM_ID ]; then
    echo "USAGE ERROR: Must specify either \`--platform\` or \`--platform-id\`!"
    exit 5
elif [ ! -z $PLATFORM ]; then
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
            exit 6
            ;;
    esac
else
    case "$PLATFORM_ID" in
        0)
            PLATFORM="core"
            ;;
        6)
            PLATFORM="photon"
            ;;
        8)
            PLATFORM="p1"
            ;;
        10)
            PLATFORM="electron"
            ;;
        *)
            echo "ERROR: No rules to release platform id: $PLATFORM_ID!"
            exit 7
            ;;
    esac
fi

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

if [ $DEBUG = true ]; then
    BINARY_DIRECTORY=$QUALIFIED_OUTPUT_DIRECTORY/debug
else
    BINARY_DIRECTORY=$QUALIFIED_OUTPUT_DIRECTORY/release
fi
mkdir -p $BINARY_DIRECTORY

OUT_CORE=$ABSOLUTE_TARGET_DIRECTORY/main/platform-$PLATFORM_ID-lto
OUT_MODULE=$ABSOLUTE_TARGET_DIRECTORY/user-part/platform-$PLATFORM_ID-m

# Cleanup
rm -rf ../build/modules/
rm -rf $ABSOLUTE_TARGET_DIRECTORY/

# Build Platform Bootloader
cd ../bootloader
make clean all -s PLATFORM_ID=$PLATFORM_ID
release_binary bootloader lto

# Photon (6), P1 (8)
if [ $PLATFORM_ID -eq 6 ] || [ $PLATFORM_ID -eq 8 ]; then
    cd ../modules
    if [ $DEBUG = true ]; then
        make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=y USE_SWD_JTAG=y
    else
        make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n
        release_binary_module user-part tinker
    fi
    release_binary system-part1
    release_binary system-part2

# Electron (10)
elif [ $PLATFORM_ID -eq 10 ]; then
    cd ../modules
    if [ $DEBUG = true ]; then
        make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=y USE_SWD_JTAG=y
    else
        make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=y
        release_binary_module user-part tinker
    fi
    release_binary system-part1
    release_binary system-part2
    release_binary system-part3

# Core (0)
elif [ $PLATFORM_ID -eq 0 ]; then
    cd ../main
    if [ $DEBUG = true ]; then
        make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=y USE_SWD_JTAG=y APP=tinker
    else
        make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=y APP=tinker
    fi
    release_binary_core tinker
    cd ../modules
fi

# Scrub release directory
if [ $DEBUG = false ]; then
    rm $BINARY_DIRECTORY/*.elf
    rm $BINARY_DIRECTORY/*.hex
    rm $BINARY_DIRECTORY/*.lst
    rm $BINARY_DIRECTORY/*.map
fi
