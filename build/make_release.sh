#!/bin/bash
set -o errexit -o pipefail -o noclobber -o nounset

function display_help ()
{
    echo "\
usage: make_release.sh [--debug] [--help]
                       [--output-directory=<binary_output_directory>]
                       [--platform=<all|argon|asom|boron|bsom...
                       |core|electron|p1|photon|xenon|xsom>]
                       [--publish=<semantic_version_string>] [--tests]

Generate the binaries for a versioned release of the Device OS. This utility
is capable of generating both debug and release binaries, as well as the
associated tests for a specified platform.

  -d, --debug             Generate debug binaries for the selected platform.
  -h, --help              Display this help and exit.
  -o, --output-directory  Specify the root output directory where the
                            folder hierarchy for the resulting binaries
                            will be placed. If not specified, the resulting
                            binaries will be placed in '<particle-iot/device-os>...
                            /build/releases/' by default.
  -p, --platform          Specify the desired platform. If no platform is
                            specified, then 'all' will be defaulted.
  --publish               Collect files into a publish directory.
  -t, --tests             Generate test binaries for the selected platform.
"
}

# Utilized Enhanced `getopt`
! getopt --test > /dev/null
if [ ${PIPESTATUS[0]} -ne 4 ]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

OPTIONS=dho:p:t
LONGOPTS=debug,help,output-directory:,platform:,publish:,tests

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
GENERATE_TESTS=false
OUTPUT_DIRECTORY="../build/releases"
PLATFORM="all"
PUBLISH=""

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
        --publish)
            PUBLISH="$2"
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

function release_platform()
{
    # Parse parameter(s)
    platform=$1

	# Generate release binaries
	./release.sh --platform "$platform" $PASSTHROUGH_FLAGS --output-directory "$OUTPUT_DIRECTORY"

	# Generate debug binaries
	if [ $DEBUG = true ]; then
		./release.sh --platform "$platform" --debug --output-directory "$OUTPUT_DIRECTORY"
	fi
}

function valid_platform()
{
    # Parse parameter(s)
    platform=$1

    # Validate platform (result of expression returned to caller)
    [ "$platform" = "all" ] || [ "$platform" = "argon" ] || [ "$platform" = "asom" ] || [ "$platform" = "boron" ] || [ "$platform" = "bsom" ] || [ "$platform" = "core" ] || [ "$platform" = "electron" ] || [ "$platform" = "p1" ] || [ "$platform" = "photon" ] || [ "$platform" = "xenon" ] || [ "$platform" = "xsom" ]
}

if !(valid_platform $PLATFORM); then
	echo "ERROR: Invalid platform \"$PLATFORM\"!"
	exit 5
fi

# Prepare pass through flags
PASSTHROUGH_FLAGS=""
if [ $GENERATE_TESTS = true ]; then
	PASSTHROUGH_FLAGS+=" --tests"
fi

# Release platform(s)
if [ $PLATFORM = "all" ]; then
	release_platform "argon"
	release_platform "asom"
	release_platform "boron"
	release_platform "bsom"
	release_platform "core"
	release_platform "electron"
	release_platform "p1"
	release_platform "photon"
	release_platform "xenon"
	release_platform "xsom"
else
	release_platform "$PLATFORM"
fi

# Publish binaries
if [ ! -z $PUBLISH ]; then
	./release-publish.sh --release-directory $OUTPUT_DIRECTORY --version $PUBLISH
fi
