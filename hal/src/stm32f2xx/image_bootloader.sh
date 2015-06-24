
bl_dir=../../../build/target/bootloader

function update_bl()
{
   cp $bl_dir/platform-$1-lto/bootloader.bin bootloader_platform_$1.bin
   xxd -i bootloader_platform_$1.bin > bootloader_platform_$1.c
   rm bootloader_platform_$1.bin
   sed -i .bak 's/unsigned/const unsigned/' bootloader_platform_$1.c
}
update_bl 6
update_bl 8
