# pulls the data from the spark external flash

set target=cores/%1
mkdir -p $target

set usbid=2b04:d006
rem 1d50:607f

dfu-util -d %usbid% -a 1 -s 1634:128 -v -U %target%/server_address_pull
dfu-util -d %usbid% -a 1 -s 1250:294 -v -U %target%/public_key_pull
dfu-util -d %usbid% -a 1 -s 34:612 -v -U %target%/private_key_pull

openssl rsa -in %target%/private_key_pull -inform der -pubout > %target%/core.pub.pem
