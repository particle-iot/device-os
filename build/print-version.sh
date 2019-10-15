#!/bin/bash

VERSION_FILE="../version.semver.json"

# Confirm version file exists and is readable
if [ ! -r "$VERSION_FILE" ]; then
    echo "ERROR: Cannot locate version file, \"$VERSION_FILE\"!";
    exit 1;
fi

function json () {
    local object_string=$1
    local member=$2
    echo $object_string | base64 --decode | jq --raw-output $member
}

VERSION_OBJECT=$(jq --raw-output '.semver.version | @base64' ${VERSION_FILE})

MAJOR=$(json $VERSION_OBJECT .major)
MINOR=$(json $VERSION_OBJECT .minor)
PATCH=$(json $VERSION_OBJECT .patch)
DOT_SEPARATED_PRE_RELEASE_IDENTIFIERS=""
DOT_SEPARATED_BUILD_METADATA_IDENTIFIERS=""

# Generate dot-seperated pre-release identifiers
for pre_release_identifier in $(json $VERSION_OBJECT .[\"pre-release\"][]); do
    if [ ! -e $DOT_SEPARATED_PRE_RELEASE_IDENTIFIERS ]; then
        DOT_SEPARATED_PRE_RELEASE_IDENTIFIERS+=".";
    fi
    DOT_SEPARATED_PRE_RELEASE_IDENTIFIERS+="$pre_release_identifier";
done

# Generate dot-seperated pre-release identifiers
for build_metadata_identifier in $(json $VERSION_OBJECT .[\"build-metadata\"][]); do
    if [ ! -e $DOT_SEPARATED_BUILD_METADATA_IDENTIFIERS ]; then
        DOT_SEPARATED_BUILD_METADATA_IDENTIFIERS+=".";
    fi
    DOT_SEPARATED_BUILD_METADATA_IDENTIFIERS+="$build_metadata_identifier";
done

# print version
VERSION="${MAJOR}.${MINOR}.${PATCH}"
if [ ! -z $DOT_SEPARATED_PRE_RELEASE_IDENTIFIERS ]; then
    VERSION="$VERSION-$DOT_SEPARATED_PRE_RELEASE_IDENTIFIERS"
fi
if [ ! -z $DOT_SEPARATED_BUILD_METADATA_IDENTIFIERS ]; then
    VERSION="$VERSION+$DOT_SEPARATED_BUILD_METADATA_IDENTIFIERS"
fi
echo $VERSION