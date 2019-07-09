#!/bin/bash
set -o errexit -o pipefail -o noclobber -o nounset

VERSION="1.2.1"

function display_help ()
{
    echo "\
usage: release.sh [--output-directory=<binary_output_directory>]
                  (--platform=<argon|asom|boron|bsom...
                  |core|electron|p1|photon|xenon|xsom>...
                  | --platform-id=<0|6|8|10|12|13|14|22|23|24>)
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
                            binaries will be placed in '<particle-iot/device-os>...
                            /build/releases/' by default.
  -p, --platform          Specify the desired platform.
  -t, --tests             Generate test binaries for the current platform.
"
}

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
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
    if [ "$suffix" = "lto" ]; then
        compile_lto="y"
    fi

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
    if [ $DEBUG = true ]; then
        release_file "$from_name" "$to_name" "elf" "$suffix" "$debug_build" "$use_swd_jtag"
        release_file "$from_name" "$to_name" "hex" "$suffix" "$debug_build" "$use_swd_jtag"
        release_file "$from_name" "$to_name" "lst" "$suffix" "$debug_build" "$use_swd_jtag"
        release_file "$from_name" "$to_name" "map" "$suffix" "$debug_build" "$use_swd_jtag"
    fi
}

# Align platform data (prefer name)
if [ -z $PLATFORM ] && [ -z $PLATFORM_ID ]; then
    echo "USAGE ERROR: Must specify either \`--platform\` or \`--platform-id\`!"
    exit 5
elif [ ! -z $PLATFORM ]; then
    case "$PLATFORM" in
        "core")
            PLATFORM_ID="0"
            MESH=false
            ;;
        "photon")
            PLATFORM_ID="6"
            MESH=false
            ;;
        "p1")
            PLATFORM_ID="8"
            MESH=false
            ;;
        "electron")
            PLATFORM_ID="10"
            MESH=false
            ;;
        "argon")
            PLATFORM_ID="12"
            MESH=true
            ;;
        "boron")
            PLATFORM_ID="13"
            MESH=true
            ;;
        "xenon")
            PLATFORM_ID="14"
            MESH=true
            ;;
        "asom")
            PLATFORM_ID="22"
            MESH=true
            ;;
        "bsom")
            PLATFORM_ID="23"
            MESH=true
            ;;
        "xsom")
            PLATFORM_ID="24"
            MESH=true
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
            MESH=false
            ;;
        6)
            PLATFORM="photon"
            MESH=false
            ;;
        8)
            PLATFORM="p1"
            MESH=false
            ;;
        10)
            PLATFORM="electron"
            MESH=false
            ;;
        12)
            PLATFORM="argon"
            MESH=true
            ;;
        13)
            PLATFORM="boron"
            MESH=true
            ;;
        14)
            PLATFORM="xenon"
            MESH=true
            ;;
        22)
            PLATFORM="asom"
            MESH=true
            ;;
        23)
            PLATFORM="bsom"
            MESH=true
            ;;
        24)
            PLATFORM="xsom"
            MESH=true
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

# Core (0)
if [ $PLATFORM_ID -eq 0 ]; then
    # Configure
    cd ../main
    DEBUG_BUILD="n"
    MODULAR="n"

    # Compose, echo and execute the `make` command
    MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=y DEBUG_BUILD=$DEBUG_BUILD MODULAR=$MODULAR USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n APP=tinker"
    echo $MAKE_COMMAND
    eval $MAKE_COMMAND

    # Migrate file(s) into output interface
    release_binary "main" "tinker" "-lto" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    cd ../modules

# Photon (6), P1 (8)
elif [ $PLATFORM_ID -eq 6 ] || [ $PLATFORM_ID -eq 8 ]; then
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

    # Compose, echo and execute the `make` command
    MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=$DEBUG_BUILD MODULAR=$MODULAR USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n"
    if [ "$MODULAR" = "n" ]; then
        MAKE_COMMAND+=" APP=tinker-serial1-debugging"
    else
        MAKE_COMMAND+=" APP=tinker"
    fi
    echo $MAKE_COMMAND
    eval $MAKE_COMMAND

    # Migrate file(s) into output interface
    if [ "$MODULAR" = "n" ]; then
        release_binary "tinker-serial1-debugging" "tinker-serial1-debugging" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    else
        release_binary "system-part1" "system-part1" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
        release_binary "system-part2" "system-part2" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
        release_binary "user-part" "tinker" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    fi

# Electron (10)
elif [ $PLATFORM_ID -eq 10 ]; then
    # Configure
    cd ../modules
    DEBUG_BUILD="y"
    MODULAR="y"

    # Compose, echo and execute the `make` command
    MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=$DEBUG_BUILD MODULAR=$MODULAR USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n APP=tinker"
    echo $MAKE_COMMAND
    eval $MAKE_COMMAND

    # Migrate file(s) into output interface
    release_binary "system-part1" "system-part1" "-m" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    release_binary "system-part2" "system-part2" "-m" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    release_binary "system-part3" "system-part3" "-m" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    release_binary "user-part" "tinker" "-m" "$DEBUG_BUILD" "$USE_SWD_JTAG"

# Mesh
elif [ $PLATFORM_ID -eq 12 ] || [ $PLATFORM_ID -eq 13 ] || [ $PLATFORM_ID -eq 14 ] || [ $PLATFORM_ID -eq 22 ] || [ $PLATFORM_ID -eq 23 ] || [ $PLATFORM_ID -eq 24 ]; then
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

    # Compose, echo and execute the `make` command
    MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=n DEBUG_BUILD=$DEBUG_BUILD MODULAR=$MODULAR USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n"
    if [ "$MODULAR" = "n" ]; then
        MAKE_COMMAND+=" APP=tinker-serial1-debugging"
    else
        MAKE_COMMAND+=" APP=tinker"
    fi
    echo $MAKE_COMMAND
    eval $MAKE_COMMAND

    # Migrate file(s) into output interface
    if [ "$MODULAR" = "n" ]; then
        release_binary "tinker-serial1-debugging" "tinker-serial1-debugging" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    else
        release_binary "system-part1" "system-part1" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
        release_binary "user-part" "tinker" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
    fi

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
if [ $MESH = true ]; then
    COMPILE_LTO="n"
    DEBUG_BUILD="n"
    SUFFIX=""
else
    COMPILE_LTO="y"
    SUFFIX="-lto"
fi

# Compose, echo and execute the `make` command
MAKE_COMMAND="make -s clean all PLATFORM_ID=$PLATFORM_ID COMPILE_LTO=$COMPILE_LTO DEBUG_BUILD=$DEBUG_BUILD USE_SWD_JTAG=$USE_SWD_JTAG USE_SWD=n"
echo $MAKE_COMMAND
eval $MAKE_COMMAND

# Migrate file(s) into output interface
release_binary "bootloader" "bootloader" "$SUFFIX" "$DEBUG_BUILD" "$USE_SWD_JTAG"
