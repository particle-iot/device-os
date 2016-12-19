# updates the bootloader image embedded in system firmware
bl_dir=../../../build/target/bootloader

platform=$(uname)

function die()
{
echo "Something very bad happened. Exiting now to avoid a catastrophic calamity."
}

function update_bl()
{
# $1 - platform ID
   pushd ../../../bootloader
   make -s PLATFORM_ID=$1 clean all || die   
   popd
   cp $bl_dir/platform-$1-lto/bootloader.bin bootloader_platform_$1.bin || die
   xxd -i bootloader_platform_$1.bin > bootloader_platform_$1.c || die
   rm bootloader_platform_$1.bin || die
   if [[ "$platform" == "Darwin" ]]; then
     sed -i '' 's/unsigned/const unsigned/' bootloader_platform_$1.c || die
   else
     sed -i 's/unsigned/const unsigned/' bootloader_platform_$1.c || die
   fi
}
update_bl 6
update_bl 8
update_bl 10
