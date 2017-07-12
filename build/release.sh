VERSION="0.7.0-rc.2"

function release_file()
{
	name=$1
	ext=$2
	cp ../build/target/$name/platform-$PLATFORM_ID-m/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_file_core()
{
	name=$1
	ext=$2
	cp $OUT_CORE/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_binary()
{
	release_file $1 bin
	cp $OUT/$1-$VERSION-$PLATFORM.bin $BINARIES_OUT/$1-$VERSION-$PLATFORM.bin
	release_file $1 elf
	release_file $1 map
	release_file $1 lst
	release_file $1 hex
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
	make -s PLATFORM_ID=$PLATFORM_ID clean all COMPILE_LTO=n
	release_binary system-part1
	release_binary system-part2
else if [ $1 -eq 10 ]; then
	make -s PLATFORM_ID=$PLATFORM_ID clean all COMPILE_LTO=n DEBUG_BUILD=y # APP=tinker_electron
	release_binary system-part1
	release_binary system-part2
	release_binary system-part3
else if [ $1 -eq 0 ]; then
	cd ../main
	make -s PLATFORM_ID=$PLATFORM_ID clean all COMPILE_LTO=y APP=tinker
	release_binary_core tinker
	cd ../modules
fi
fi
fi