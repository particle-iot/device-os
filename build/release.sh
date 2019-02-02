#!/bin/bash
set -o errexit -o pipefail -o noclobber -o nounset

VERSION="1.0.1"

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

OPTIONS=i:p:o:
LONGOPTS=platform-id:,platform:,output-directory:

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
PLATFORM=""
PLATFORM_ID=""
OUTPUT_DIRECTORY="../build/releases"

# Parse parameter(s)
while true; do
    case "$1" in
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
    name=$1
    ext=$2
    suffix=$3

    # Move file from build to release folder
    cp ../build/target/$name/platform-$PLATFORM_ID-$suffix/$name.$ext $RELEASE_DIR/$name-$VERSION+$PLATFORM.$ext
}

function release_binary()
{
    # Parse parameter(s)
    name=$1
    suffix=${2:-m}

    # Move files into release folder
    release_file $name bin $suffix
    release_file $name elf $suffix
    release_file $name map $suffix
    release_file $name lst $suffix
    release_file $name hex $suffix

    # Move release binaries to publish folder
    cp $RELEASE_DIR/$name-$VERSION+$PLATFORM.bin $TEMPORARY_DIR/
}

function release_file_core()
{
    # Parse parameter(s)
    name=$1
    ext=$2

    # Move file from build target to release folder
    cp $OUT_CORE/$name.$ext $RELEASE_DIR/$name-$VERSION+$PLATFORM.$ext
}

function release_binary_core()
{
    # Parse parameter(s)
    name=$1

    release_file_core $name bin
    release_file_core $name elf
    release_file_core $name map
    release_file_core $name lst
    release_file_core $name hex

    # Move release binaries to publish folder
    cp $RELEASE_DIR/$name-$VERSION+$PLATFORM.bin $TEMPORARY_DIR/
}

function release_binary_module()
{
    # Parse parameter(s)
    source_name=$1
    target_name=$2

    # Move file from build target to release folder
    cp $OUT_MODULE/$source_name.bin $TEMPORARY_DIR/$target_name-$VERSION+$PLATFORM.bin
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

# Create binary output directories
BUILD_ROOT=pwd

QUALIFIED_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/$VERSION/$PLATFORM
TEMPORARY_DIR=$QUALIFIED_OUTPUT_DIRECTORY/tmp
mkdir -p $TEMPORARY_DIR

RELEASE_DIR=$QUALIFIED_OUTPUT_DIRECTORY/release
mkdir -p $RELEASE_DIR

GITHUB_RELEASE_DIRECTORY=$OUTPUT_DIRECTORY/$VERSION/github
mkdir -p $GITHUB_RELEASE_DIRECTORY

OUT_CORE=../build/target/main/platform-$PLATFORM_ID-lto
OUT_MODULE=../build/target/user-part/platform-$PLATFORM_ID-m

# Cleanup
rm -rf ../build/modules/
rm -rf ../build/target/

# Build Platform Bootloader
cd ../bootloader
make clean all -s PLATFORM_ID=$PLATFORM_ID
release_binary bootloader lto

# Photon (6), P1 (8)
if [ $PLATFORM_ID -eq 6 ] || [ $PLATFORM_ID -eq 8 ]; then
    cd ../modules
    make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n
    release_binary system-part1
    release_binary system-part2
    release_binary_module user-part tinker

# Electron (10)
elif [ $PLATFORM_ID -eq 10 ]; then
    cd ../modules
    make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=y
    release_binary system-part1
    release_binary system-part2
    release_binary system-part3
    release_binary_module user-part tinker

# Core (0)
elif [ $PLATFORM_ID -eq 0 ]; then
    cd ../main
    cp ../Dockerfile.test .
    make clean all -s PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=y APP=tinker
    release_binary_core tinker
    cd ../modules
fi

# Create tarball for publication
pushd $TEMPORARY_DIR
tar -czvf $GITHUB_RELEASE_DIRECTORY/particle-$VERSION+$PLATFORM.tar.gz *.bin
mv *.bin $GITHUB_RELEASE_DIRECTORY
popd
rm -rf $TEMPORARY_DIR
