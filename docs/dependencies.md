## 1. Download and Install Dependencies 

Building the firmware locally requires these dependencies ot be installed:

1. [GCC for ARM Cortex processors](#1-gcc-for-arm-cortex-processors)
2. [Make](#2-make)
3. [Device Firmware Upgrade Utilities](#3-device-firmware-upgrade-utilities)
4. [Zatig](#4-zatig) (for windows users only)
5. [Git](#5-git)


#### 1. GCC for ARM Cortex processors
The Core/Photon uses an ARM Cortex M3 CPU based microcontroller. All of the code is built around the GNU GCC toolchain offered and maintained by ARM.  

Linux And Windows
- Download and install version 4.9.x from: https://launchpad.net/gcc-arm-embedded

OS X:
 - `brew install gcc-arm-none-eabi-49`

#### 2. Make 
In order to turn your source code into binaries, you will need a tool called `make`. Windows users need to explicitly install `make` on their machines. Make sure you can use it from the terminal window.

Download and install the latest version from: http://gnuwin32.sourceforge.net/packages/make.htm

#### 3. Device Firmware Upgrade Utilities
Install dfu-util 0.8. Mac users can install dfu-util with [Homebrew](http://brew.sh/) or [Macports](http://www.macports.org), Linux users may find it in their package manager, and everyone can get it from http://dfu-util.gnumonks.org/index.html

#### 4. Zatig
In order for the device to show up on the dfu list, you need to replace the USB driver with a utility called [Zadig](http://zadig.akeo.ie/). Here is a [tutorial](https://community.spark.io/t/tutorial-installing-dfu-driver-on-windows/3518) on using it. This is only required for Windows users.

#### 5. Git

Download and install Git: http://git-scm.com/

#### 6. Command line tools

- crc32
 - available in MinGW on Windows 
 - available by default on OS X
 - linux users, please check with your package manager


