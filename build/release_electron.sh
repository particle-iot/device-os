VERSION=0.0.3-rc.1

function release_file()
{
	name=$1
	ext=$2
   cp ../build/target/$name/platform-$PLATFORM_ID-m/$name.$ext $OUT/$name-$VERSION-$PLATFORM.$ext
}

function release_binary()
{
	release_file $1 bin
	release_file $1 elf
	release_file $1 map
	release_file $1 lst
	release_file $1 hex
}

PLATFORM_ID=$1
PLATFORM=$2
cd ../modules


OUT=../build/releases/release-$VERSION-p$PLATFORM_ID
mkdir -p $OUT
rm -rf ../build/target
make -s PLATFORM_ID=$PLATFORM_ID clean all COMPILE_LTO=n DEBUG_BUILD=y APP=tinker_electron
release_binary system-part1
release_binary system-part2

