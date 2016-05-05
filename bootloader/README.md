# Particle STM32 Bootloader

## Photon/Electron

### OTA

After building the bootloader, it can be flashed to the device Over-the-Air using Particle CLI.

```
particle flash <devicename> bootloader.bin
```

This takes care of unlocking the protected memory regions, flashing the bootloader, and reprotecting the memory regions. 

### Using the Programmer Shield

- ensure that the environment variable `OPENOCD_HOME` points to the directory containing OpenOCD on your computer. 
- connect the programmer shield and start a telnet session on port 3333 as described [here](https://github.com/spark/shields/tree/master/photon-shields/programmer-shield)
- send the openOCD commands via telnet
```
reset halt
flash protect 0 0 off
``` 
- in the bootloader directory run
```
make PLATFORM=photon all program-openocd
```
- finally send the telnet command
```
flash protect 0 0 on
```


## Core

*Careful!  Low level dragons here!*

  * Really make sure you have a working programmer shield and an ST-link programmer.
  * Is this a brand new core from scratch (that you made? nice!) goto 1, or is it this a core that came from Particle - goto "unlocking your bootloader"

Please note this documentation is for the Core only. The photon automaatically updates the bootloader as needed.

### Unlocking your bootloader

Welcome! The bootloader on the stm32 is protected by a "write protect" flag that helps prevent accidental "bricking" of a core for those without a programmer shield / st-link programmer. But that's not you, you want to fully erase your core, or re-write your bootloader, and you're not scared of "bricking" anything.

 * Grab the "unlocker" from here: (https://github.com/spark/firmware/tree/master/bootloader/tools (use "raw")
 * dfu that with a:  `dfu-util -d 1d50:607f -a 0 -s 0x08005000:leave -D unlocker-firmware.bin`
 * Your core should restart, flash some lights, and end with a 'green' flash before wiping saved wifi profiles and resetting
 * Your bootloader is unlocked!

**1. Make sure you have st-flash installed, or have a copy of the ST-Link Utility software.**

 * You can find the software on their site if you're using windows: http://www.st.com/web/catalog/tools/FM146/CL1984/SC724/SS1677/PF251168

 * If you're on mac / linux grab "st-flash" you can grab and build a tool yourself:
    ```
    git clone git@github.com:texane/stlink.git
    cd stlink
    ./autogen.sh
    ./configure
    make
    make install
    ```

**2. Grab a copy of the latest bootloader from here:**

  https://github.com/spark/firmware/blob/master/bootloader/build/bootloader.bin?raw=true5.) With your core in your programmer shield / st-link connected, lets re-flash some stuff!

 * In the windows utility set device -> settings to "connect under reset" true! (if you can't find this that's okay)
 * Do a full erase of your core, do this a few times, if you're in the windows utility, view the memory and make sure it's zero-ed out.
 * Write your bootloader.bin back to 0x08000000
 * Congrats, you re-wrote your bootloader!

**3. Lock your bootloader back up! (Optional, but recommended)**

 * Grab the locking firmware here:  (https://github.com/spark/firmware/tree/master/bootloader/tools (use "raw")
 * dfu that to `dfu-util -d 1d50:607f -a 0 -s 0x08005000:leave -D locker-firmware.bin`
 * your core should reset, flash some lights, and flash 'red' when locked.

**4. Okay, lets keep going and update the radio driver on the CC3000**

 * Grab the patch programmer from here: https://github.com/spark/cc3000-patch-programmer/blob/master/build/cc3000-patch-programmer.bin?raw=true
 * dfu it with: `dfu-util -d 1d50:607f -a 0 -s 0x08005000:leave -D cc3000-patch-programmer.bin`
 * hold down the mode button for 3+ seconds until the light starts blinking, let your core sit for 20-60 seconds after the light stops blinking.
 * At this point you can erase your core with the st-link programmer (did you lock your bootloader?), to get it back into dfu mode


### It's time to write some stuff to external flash!

 **1. Lets make some keys (optional step if your keys have been fine):**

```
    openssl genrsa -out core.pem 1024
    openssl rsa -in core.pem -pubout -out core_public.pem
    openssl rsa -in core.pem -outform DER -out core_private.der
```

 **2. Flash your new private key:**

    `dfu-util -d 1d50:607f -a 1 -s 0x00002000 -v -D core_private.der`

 **3. Grab the server public key, and flash it:**
    https://s3.amazonaws.com/spark-website/cloud_public.der

    `dfu-util -d 1d50:607f -a 1 -s 0x00001000 -v -D cloud_public.der`

 **4. Grab the latest firmware and flash it**

    https://github.com/spark/firmware/blob/master/main/build/core-firmware.bin?raw=true

```
    # to the factory reset spot
    dfu-util -a 1 -s 0x00020000 -v -D core-firmware.bin

    ## to the main firmware spot
    dfu-util -a 0 -s 0x08005000:leave -D core-firmware.bin
```

 **5. If you made new keys, be sure to send someone at Spark your new public key, along with your core id, otherwise your core won't connect to the cloud again!**

 *(Thanks to David for putting this together)*
 Ref: https://community.spark.io/t/st-link-programmer-shield-only-so-youve-decided-you-want-to-update-your-bootloader-and-everything/2355
