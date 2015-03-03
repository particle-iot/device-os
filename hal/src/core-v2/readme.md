# Welcome, Alpha Photon Users!

Here's a getting started guide to get your photon connected to the cloud and
the commands needed to build and flash firmware.

The guide covers these points:

- [Environment Setup]()
- [Provisioning Keys]()
- [Building and flashing the bootloader]()
- [Building and flashing firmware]()


# Environment Setup

## Download and Install Dependencies 

1. [GCC for ARM Cortex processors](#1-gcc-for-arm-cortex-processors)
2. [Make](#2-make)
3. [Device Firmware Upgrade Utilities](#3-device-firmware-upgrade-utilities)
4. [Zatig](#4-zatig) (for windows users only)
5. [Git](#5-git)
6. [openssl](#6-openssl)

#### 1. GCC for ARM Cortex processors
The Photon uses an ARM Cortex M3 CPU based microcontroller. All of the code is built around the GNU GCC toolchain offered and maintained by ARM.  

Download and install the latest version from: https://launchpad.net/gcc-arm-embedded

See [this Gist](https://gist.github.com/joegoggins/7763637) for how to get setup on OS X.

#### 2. Make 
In order to turn your source code into binaries, you will need a tool called `make`. Windows users need to explicitly install `make` on their machines. Make sure you can use it from the terminal window.

Download and install the latest version from: http://gnuwin32.sourceforge.net/packages/make.htm

#### 3. Device Firmware Upgrade Utilities
Install dfu-util 0.7. Mac users can install dfu-util with [Homebrew](http://brew.sh/) or [Macports](http://www.macports.org), Linux users may find it in their package manager, and everyone can get it from http://dfu-util.gnumonks.org/index.html

#### 4. Zatig
In order for the Photon to show up on the dfu list, you need to replace the USB driver with a utility called [Zadig](http://zadig.akeo.ie/). Here is a [tutorial](https://community.spark.io/t/tutorial-installing-dfu-driver-on-windows/3518) on using it. This is only required for Windows users.

#### 5. Git
Download and install Git: http://git-scm.com/

#### 6. OpenSSL

This is pre-installed on linux and OS X - windows users can find it here - https://www.openssl.org/related/binaries.html.

### Clone this repo

`git clone http://github.com/spark/firmware-private`


## Provisioning Keys

Before the device can connect to the cloud the server public key needs to be installed
and the device public key registered with the cloud.

### Extracting Generated Keys

The Photon generates a keypair for communicating cloud. In production versions,
this will be sent to the cloud automatically. For this alpha version, the keys are extracted
and sent to the cloud by hand. 

With the Photon in dfu mode, extract the public keys:
```
dfu-util -d 2b04:d006 -a 1 -s 1250:294 -v -U photon_public.der
```

The cloud requires the keys in PEM format. The pubic key is converted with:

```
openssl rsa -pubin -in photon_public.der -inform DER -outform PEM -out photon.pub.pem
```

### Upload the Server public Key to the Device

Download the server public key from https://s3.amazonaws.com/spark-website/cloud_public.der 

and upload this to the device:

```
dfu-util -d 2b04:d006 -a 1 -s 2082 -D cloud_public.der
```

### Registering your Photon's Public Key

To register the public key, we need the device ID. First reset the device - it will start by entering setup mode (flashing blue LED.)

Then fetch the device ID, either using

```
spark serial identify
```

or by connecting to the device using a serial terminal, and typing 'i'.

Then email the device ID and the `photon.pub.pem` file to `mat _at_ spark.io` to register the device.

### Enter WiFi Details

If you are using `spark-cli`

```
spark serial wifi
```

Otherwise wifi credentials are set the same as with the Core, by opening a terminal 
to the device's serial port and typing `w`, and following the prompts.

See, told you it would be easy!

### Connect to the Cloud

Once the public key has been registered the device should connect to the cloud (breathing cyan - YAY!)

Then it's onward to building the latest and greatest bootloader and firmware!


# Building and Flashing the Bootloader

### Building the bootloader

From the root of the repo:

```
cd bootloader
make PLATFORM=photon all
```

You should see output similar to:

```
   text    data     bss     dec     hex filename
  15532     360    6260   22152    5688 ../build/target/bootloader/platform-6/bootloader.elf
```

### Flashing the bootloader

Using st-link, the bootloader can be flashed using the `st-flash` goal:

```
make PLATFORM=photon st-all flash
```

or with the command:

```
st-flash write ../build/target/bootloader/platform-6/bootloader.elf 0x8000000
```


# Building and Flashing Firmware

The documentation for the build process can be found in [/build/readme.md](../../../build/readme.md)
You'll find all the details about building firmware and custom apps there -
here's a quick into to get you started!

### Building Firmware

To build Tinker, from the root of the repo:

```
cd main
make PLATFORM=photon APP=tinker
```

You should see output similar to

```
   text    data     bss     dec     hex filename
 363352    1696   51116  416164   659a4 ../build/target/main/platform-6/applications/tinker/tinker.elf
```

### Flashing Firmware

There are two ways to flash firmware - via DFU or via JTAG

- DFU: first place the device in DFU mode and add `program-dfu` to the command above
- st-link JTAG: add `st-flash` to the command above

And that's it! You should be up and running and able to create firmware.

Please note that a this is alpha, there are some features of Wiring that are not presently implemented:

- IPv6 support
- TCP Server
- UDP

Any questions - let me know! mat _at_ spark.io