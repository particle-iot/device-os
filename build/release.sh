VERSION="0.8.0-rc.1"

function release_file()
{
	name=$1
	ext=$2
        suffix=$3
	cp ../build/target/$name/platform-$PLATFORM_ID-$suffix/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_file_core()
{
	name=$1
	ext=$2
	cp $OUT_CORE/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_binary()
{
	suffix=${2:-m}
	release_file $1 bin $suffix
	cp $OUT/$1-$VERSION-$PLATFORM.bin $BINARIES_OUT/$1-$VERSION-$PLATFORM.bin
	release_file $1 elf $suffix
	release_file $1 map $suffix
	release_file $1 lst $suffix
	release_file $1 hex $suffix
}

function release_binary_core()
{
	release_file_core $1 bin
	cp $OUT_CORE/$1.bin $BINARIES_OUT/$1-$VERSION-$PLATFORM.bin
	release_file_core $1 elf
	release_file_core $1 map
	release_file_core $1 lst
	release_file_core $1 hex
}

PLATFORM_ID=$1
PLATFORM=$2
cd ../modules

# convenient place to grab binaries for github release
BINARIES_OUT=../build/releases/$VERSION
mkdir -p $BINARIES_OUT

OUT_CORE=../build/target/main/platform-$PLATFORM_ID-lto
OUT=../build/releases/release-$VERSION-p$PLATFORM_ID
mkdir -p $OUT
rm -rf ../build/target
if [ $1 -eq 6 ] || [ $1 -eq 8 ]; then
        make -s -C ../bootloader PLATFORM_ID=$PLATFORM_ID all
	release_binary bootloader lto
	make -s PLATFORM_ID=$PLATFORM_ID all COMPILE_LTO=n
	release_binary system-part1
	release_binary system-part2
else if [ $1 -eq 10 ]; then
        make -s -C ../bootloader PLATFORM_ID=$PLATFORM_ID  all
        release_binary bootloader lto
	make -s PLATFORM_ID=$PLATFORM_ID all COMPILE_LTO=n DEBUG_BUILD=y # APP=tinker_electron
	release_binary system-part1
	release_binary system-part2
	release_binary system-part3
else if [ $1 -eq 0 ]; then
	cd ../main
cp ../Dockerfile.test .
        make -s -C ../bootloader PLATFORM_ID=$PLATFORM_ID  all 
	make -s PLATFORM_ID=$PLATFORM_ID all COMPILE_LTO=y APP=tinker
	release_binary_core tinker
	release_binary bootloader lto
	cd ../modules
fi
fi
fi
