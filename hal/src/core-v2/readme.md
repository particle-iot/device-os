### Flashing Keys

(Note that production firmware will generate it's own keys - these steps
are temporary.)

First, use the spark CLI to create keys for each device with:

`spark keys new`

if you are flashing multiple devices be sure to do this in a separate folder for each device — it always overwrites the filenames without checking! This command will generate 3 files. 

Then convert the public key to the neccasary format:  

`openssl rsa -pubin -in core.pub.pem -inform PEM  -outform DER -out public.der`  

Enter DFU mode by holding mode button while reseting the device for 3 seconds, then
flash all 3 keys to INTERNAL FLASH DCT area:

`dfu-util -d 2b04:d006 -a 1 -s 34 -D core.der`
`dfu-util -d 2b04:d006 -a 1 -s 2082 -D cloud_public.der`

You can then send the public key to the cloud using `spark keys send`.


### Flashing Firmware

- adding `program-dfu` to the make command line will flash the firmware using dfu-util
- adding `stflash` to the make command line will flash the firmware using the STLink st-flash utility
