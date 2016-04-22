if [ $1 -eq 6 ] || [ $1 -eq 8 ]; then
	VERSION="0.5.0-rc.2"
else if [ $1 -eq 10 ]; then
	VERSION="0.5.0-rc.2"
	fi
fi

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
if [ $1 -eq 6 ] || [ $1 -eq 8 ]; then
	make -s PLATFORM_ID=$PLATFORM_ID clean all COMPILE_LTO=n
else if [ $1 -eq 10 ]; then
	make -s PLATFORM_ID=$PLATFORM_ID clean all COMPILE_LTO=n DEBUG_BUILD=y # APP=tinker_electron
	fi
fi
release_binary system-part1
release_binary system-part2

