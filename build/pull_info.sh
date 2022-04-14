# pulls the data from the spark external flash

target=$1

if [ -z "$target" ]; then
   target=mycore
fi

target=target/cores/$target
mkdir -p $target

dfu-util -d 1d50:607f -a 1 -s 0x00001180:128 -v -U $target/server_address
dfu-util -d 1d50:607f -a 1 -s 0x00001000:294 -v -U $target/public_key
dfu-util -d 1d50:607f -a 1 -s 0x00002000:612 -v -U $target/private_key
