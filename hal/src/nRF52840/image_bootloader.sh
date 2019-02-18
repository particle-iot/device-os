# updates the bootloader image embedded in system firmware
bl_dir=../../../build/target/bootloader
clean=clean
platform=$(uname)

function die()
{
echo $1
echo "Something very bad happened. Exiting now to avoid a catastrophic calamity."
exit
}

function update_bl()
{
# $1 - platform ID
# $2 - platform name
   pushd ../../../bootloader
   make -s PLATFORM_ID=$1 COMPILE_LTO=n $clean all || die
   popd
   miniz_compress c $bl_dir/platform-$1/bootloader.bin bootloader_platform_$1.bin.miniz || die
   xxd -i bootloader_platform_$1.bin.miniz > ../$2/bootloader_platform_$1.c || die
   rm bootloader_platform_$1.bin.miniz || die
   if [[ "$platform" == "Darwin" ]]; then
     sed -i '' 's/unsigned/const unsigned/' ../$2/bootloader_platform_$1.c || die
   else
     sed -i 's/unsigned/const unsigned/' ../$2/bootloader_platform_$1.c || die
   fi
}

command -v miniz_compress  >/dev/null 2>&1 || die "miniz_compress not found"
update_bl 12 argon
update_bl 13 boron
update_bl 14 xenon
