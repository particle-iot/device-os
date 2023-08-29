#!/bin/bash
set -o errexit -o pipefail -o noclobber -o nounset

VERSION=${VERSION:="5.5.0"}

function display_help ()
{
    echo '
usage: release.sh [--output-directory=<binary_output_directory>]
                  (--platform=<argon|asom|boron|bsom...
                  |b5som|esomx|p2>...
                  | --platform-id=<12|13|15|22|23|25|26|32>)
                  [--debug] [--help] [--tests]

Generate the binaries for a versioned release of the Device OS. This utility
is capable of generating both debug and release binaries, as well as the
associated tests for a specified platform.

  -d, --debug             Generate debug binaries (as opposed to release).
  -h, --help              Display this help and exit.
  -i, --platform-id       Specify the desired platform id.
  -o, --output-directory  Specify the root output directory where the
                            folder hierarchy for the resulting binaries
                            will be placed. If not specified, the resulting
                            binaries will be placed in `<particle-iot/device-os>...
                            /build/releases/` by default.
  -p, --platform          Specify the desired platform.
  -t, --tests             Generate test binaries for the current platform.
'
}

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo '
`getopt --test` failed in this environment!
Please confirm "GNU getopt" is installed on this device.
'
    exit 1
fi

OPTIONS=di:ho:p:t
LONGOPTS=debug,platform-id:,help,output-directory:,platform:,tests

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
DEBUG_BUILD="n"
DEBUG=false
GENERATE_TESTS=false
PLATFORM=""
PLATFORM_ID=""
OUTPUT_DIRECTORY="../build/releases"
USE_SWD_JTAG="n"
PLATFORM_MODULAR=true

# Parse parameter(s)
while true; do
    case "$1" in
        -d|--debug)
            DEBUG_BUILD="y"
            DEBUG=true
            USE_SWD_JTAG="y"
            shift 1
            ;;
        -h|--help)
            shift
            display_help
            exit 0
            ;;
        -i|--platform-id)
            PLATFORM_ID="$2"
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
        -t|--tests)
            GENERATE_TESTS=true
            shift 1
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

function append_metadata_seperator () {
    if [ $metadata = false ]; then
        metadata=true
        qualified_filename+="+"
    else
        qualified_filename+="."
    fi
}

function compose_qualified_filename ()
{
    local name=$1
    local ext=$2
    local compile_lto=$3
    local debug_build=$4
    local use_swd_jtag=$5

    local metadata=false

    qualified_filename="${PLATFORM}-${name}@${VERSION}"

    if [ $compile_lto = "y" ]; then
        append_metadata_seperator
        qualified_filename+="lto"
    fi

    if [ $debug_build = "y" ]; then
        append_metadata_seperator
        qualified_filename+="debug"
    fi

    if [ $use_swd_jtag = "y" ]; then
        append_metadata_seperator
        qualified_filename+="jtag"
    fi

    qualified_filename+=".${ext}"
}

function release_file()
{
    # Parse parameter(s)
    local from_name=$1
    local to_name=$2
    local ext=$3
    local suffix=$4
    local debug_build=$5
    local use_swd_jtag=$6

    local compile_lto="n"
    local path=$ABSOLUTE_TARGET_DIRECTORY
    local qualified_filename=""

    # All monolithic builds
    if [ "$MODULAR" = "n" ] && [ "$from_name" != "bootloader" ]; then
        path+="/main"
    else
        path+="/${from_name}"
    fi
    path+="/platform-${PLATFORM_ID}${suffix}"

    # Translate suffix to parameter
    case "$suffix" in
        *lto*) compile_lto="y";;
    esac
    # Compose file name
    compose_qualified_filename $to_name $ext $compile_lto $debug_build $use_swd_jtag

    # Move file from build to release folder
    cp ${path}/${to_name}.${ext} ${BINARY_DIRECTORY}/${qualified_filename}
}

function release_binary ()
{
    # Parse parameter(s)
    local from_name=$1
    local to_name=$2
    local suffix=$3
    local debug_build=${4:-n}
    local use_swd_jtag=${5:-n}

    # Move files into release folder
    release_file "$from_name" "$to_name" "bin" "$suffix" "$debug_build" "$use_swd_jtag"
    release_file "$from_name" "$to_name" "elf" "$suffix" "$debug_build" "$use_swd_jtag"
    release_file "$from_name" "$to_name" "map" "$suffix" "$debug_build" "$use_swd_jtag"
    if [ $DEBUG = true ]; then
        release_file "$from_name" "$to_name" "hex" "$suffix" "$debug_build" "$use_swd_jtag"
        release_file "$from_name" "$to_name" "lst" "$suffix" "$debug_build" "$use_swd_jtag"
    fi
}

# Align platform data (prefer name)
if [ -z $PLATFORM ] && [ -z $PLATFORM_ID ]; then
    echo "USAGE ERROR: Must specify either \`--platform\` or \`--platform-id\`!"
    exit 5
elif [ ! -z $PLATFORM ]; then
    case "$PLATFORM" in
        "argon")
            PLATFORM_ID="12"
            GEN3=true
            ;;
        "boron")
            PLATFORM_ID="13"
            GEN3=true
            ;;
        "esomx")
            PLATFORM_ID="15"
            GEN3=true
            ;;
        "asom")
            PLATFORM_ID="22"
            GEN3=true
            ;;
        "bsom")
            PLATFORM_ID="23"
            GEN3=true
            ;;
        "b5som")
            PLATFORM_ID="25"
            GEN3=true
            ;;
        "tracker")
            PLATFORM_ID="26"
            GEN3=true
            ;;
        "trackerm")
            PLATFORM_ID="28"
            GEN3=true
            ;;
        "p2")
            PLATFORM_ID="32"
            GEN3=true
            ;;
        "msom")
            PLATFORM_ID="35"
            GEN3=true
            ;;
        *)
            echo "ERROR: No rules to release platform: \"$PLATFORM\"!"
            exit 6
            ;;
    esac
else
    case "$PLATFORM_ID" in
        12)
            PLATFORM="argon"
            GEN3=true
            ;;
        13)
            PLATFORM="boron"
            GEN3=true
            ;;
        15)
            PLATFORM="esomx"
            GEN3=true
            ;;
        22)
            PLATFORM="asom"
            GEN3=true
            ;;
        23)
            PLATFORM="bsom"
            GEN3=true
            ;;
        25)
            PLATFORM="b5som"
            GEN3=true
            ;;
        26)
            PLATFORM="tracker"
            GEN3=true
            ;;
        28)
            PLATFORM="trackerm"
            GEN3=true
            ;;
        32)
            PLATFORM="p2"
            GEN3=true
            ;;
        35)
            PLATFORM="msom"
            GEN3=true
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

OUT_MODULE=$ABSOLUTE_TARGET_DIRECTORY/user-part/platform-$PLATFORM_ID-m

# Cleanup
rm -rf ../build/modules/
rm -rf $ABSOLUTE_TARGET_DIRECTORY/

#########################
# Build Platform System #
#########################

# GEN3
if [ $PLATFORM_ID -eq 12 ] || [ $PLATFORM_ID -eq 13 ] || [ $PLATFORM_ID -eq 15 ] || [ $PLATFORM_ID -eq 22 ] || [ $PLATFORM_ID -eq 23 ] || [ $PLATFORM_ID -eq 25 ] || [ $PLATFORM_ID -eq 26 ] || [ $PLATFORM_ID -eq 28 ] || [ $PLATFORM_ID -eq 32 ] || [ $PLATFORM_ID -eq 35 ]; then
    # Configure
    if [ $DEBUG = true ]; then
        cd ../main
        MODULAR="n"
        SUFFIX=""
    else
        cd ../modules
        MODULAR="y"
        SUFFIX="-m"
    fi
    USE_SWD_JTAG="n"

    if [ "$MODULAR" = "n" ]; then
        declare -a apps=("tinker-serial1-debugging" "tinker-serial-debugging")
    else
        declare -a apps=("tinker")
    fi

    for app in ${apps[@]}; do
        # Compose, echo and execute the `make` command
        MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=$DEBUG_BUILD MODULAR=$MODULAR USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n"
        MAKE_COMMAND+=" APP=$app"
        echo $MAKE_COMMAND
        eval $MAKE_COMMAND

        # Migrate file(s) into output interface
        if [ "$MODULAR" = "n" ]; then
            release_binary "$app" "$app" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
        else
            release_binary "system-part1" "system-part1" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
            release_binary "user-part" "$app" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
        fi
    done

fi

# Generate test binaries for platform
if [ $GENERATE_TESTS = true ]; then
    ../build/release-tests.sh --output-directory $ABSOLUTE_OUTPUT_DIRECTORY --platform $PLATFORM --version $VERSION
fi

#############################
# Build Platform Bootloader #
#############################

# Configure
cd ../bootloader
if [ $GEN3 = true ]; then
    COMPILE_LTO="n"
    DEBUG_BUILD="n"
    SUFFIX="-m"
else
    COMPILE_LTO="y"
    if [ $PLATFORM_MODULAR = true ]; then
        SUFFIX="-m-lto"
    else
        SUFFIX="-lto"
    fi
fi

# Compose, echo and execute the `make` command
MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=$COMPILE_LTO DEBUG_BUILD=$DEBUG_BUILD USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n"
echo $MAKE_COMMAND
eval $MAKE_COMMAND

# Migrate file(s) into output interface
release_binary "bootloader" "bootloader" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"

# Prebootloader
if [ $PLATFORM_ID -eq 28 ] || [ $PLATFORM_ID -eq 32 ] || [ $PLATFORM_ID -eq 35 ]; then
cd ../bootloader/prebootloader

COMPILE_LTO="n"
DEBUG_BUILD="n"
SUFFIX="-m"

MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=$COMPILE_LTO"
echo $MAKE_COMMAND
eval $MAKE_COMMAND

# Migrate file(s) into output interface
release_binary "prebootloader-mbr" "prebootloader-mbr" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
release_binary "prebootloader-part1" "prebootloader-part1" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"

fi