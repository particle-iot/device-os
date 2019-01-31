VERSION="1.0.0"

function release_file()
{
    # Parse parameter(s)
    name=$1
    ext=$2
    suffix=$3

    # Move file from build to release folder
    cp ../build/target/$name/platform-$PLATFORM_ID-$suffix/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_binary()
{
    # Parse parameter(s)
    name=$1
    suffix=${2:-m}

    # Move files into release folder
    release_file $name bin $suffix
    cp $OUT/$name-$VERSION-$PLATFORM.bin $BINARIES_OUT/
    release_file $name elf $suffix
    release_file $name map $suffix
    release_file $name lst $suffix
    release_file $name hex $suffix
}

function release_file_core()
{
    # Parse parameter(s)
    name=$1
    ext=$2

    # Move file from build to release folder
    cp $OUT_CORE/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_binary_core()
{
    # Parse parameter(s)
    name=$1

    release_file_core $name bin
    cp $OUT_CORE/$name.bin $BINARIES_OUT/$name-$VERSION-$PLATFORM.bin
    release_file_core $name elf
    release_file_core $name map
    release_file_core $name lst
    release_file_core $name hex
}

function release_binary_module()
{
    # Parse parameter(s)
    source_name=$1
    target_name=$2

    cp $OUT_MODULE/$source_name.bin $BINARIES_OUT/$target_name-$VERSION-$PLATFORM.bin
}

# Parse parameter(s)
PLATFORM_ID=$1
PLATFORM=$2

# convenient place to grab binaries for github release
BINARIES_OUT=../build/releases/$VERSION
mkdir -p $BINARIES_OUT

OUT_CORE=../build/target/main/platform-$PLATFORM_ID-lto
OUT_MODULE=../build/target/user-part/platform-$PLATFORM_ID-m
OUT=../build/releases/release-$VERSION-p$PLATFORM_ID
mkdir -p $OUT

# Cleanup
rm -rf ../build/target

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
