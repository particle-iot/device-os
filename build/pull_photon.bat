# pulls the data from the spark external flash

set target=target/cores/%1
mkdir -p $target

dfu-util -d 1d50:607f -a 1 -s 4130:128 -v -U %target%/server_address_pull
dfu-util -d 1d50:607f -a 1 -s 2082:294 -v -U %target%/public_key_pull
dfu-util -d 1d50:607f -a 1 -s 34:612 -v -U %target%/private_key_pull
