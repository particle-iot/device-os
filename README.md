# Spark Core Firmware [![Backlog](https://badge.waffle.io/spark/core-firmware.png?label=backlog&title=backlog)](https://waffle.io/spark/core-firmware) [![Build Status](https://travis-ci.org/spark/firmware.svg)](https://travis-ci.org/spark/firmware)

This is the main source code repository of the Spark Core firmware libraries.

This firmware depends on two other libraries: the [Spark Common Library](http://www.github.com/spark/core-common-lib) and the [Spark Communication Library](http://www.github.com/spark/core-communication-lib)

1. [Download and Install Dependencies](#1-download-and-install-dependencies)
2. [Download and Build Repositories](#2-download-and-build-repositories)
3. [Edit and Rebuild](#3-edit-and-rebuild)
4. [Flash It!](#4-flash-it)
 
## 1. Download and Install Dependencies 

1. [GCC for ARM Cortex processors](#1-gcc-for-arm-cortex-processors)
2. [Make](#2-make)
3. [Device Firmware Upgrade Utilities](#3-device-firmware-upgrade-utilities)
4. [Zatig](#4-zatig) (for windows users only)
5. [Git](#5-git)


#### 1. GCC for ARM Cortex processors
The Spark Core uses an ARM Cortex M3 CPU based microcontroller. All of the code is built around the GNU GCC toolchain offered and maintained by ARM.  

Download and install the latest version from: https://launchpad.net/gcc-arm-embedded

See [this Gist](https://gist.github.com/joegoggins/7763637) for how to get setup on OS X.

#### 2. Make 
In order to turn your source code into binaries, you will need a tool called `make`. Windows users need to explicitly install `make` on their machines. Make sure you can use it from the terminal window.

Download and install the latest version from: http://gnuwin32.sourceforge.net/packages/make.htm

#### 3. Device Firmware Upgrade Utilities
Install dfu-util 0.7. Mac users can install dfu-util with [Homebrew](http://brew.sh/) or [Macports](http://www.macports.org), Linux users may find it in their package manager, and everyone can get it from http://dfu-util.gnumonks.org/index.html

#### 4. Zatig
In order for the Core to show up on the dfu list, you need to replace the USB driver with a utility called [Zadig](http://zadig.akeo.ie/). Here is a [tutorial](https://community.spark.io/t/tutorial-installing-dfu-driver-on-windows/3518) on using it. This is only required for Windows users.

#### 5. Git

Download and install Git: http://git-scm.com/

## 2. Download and Build Repositories

The entire Spark Core firmware is organized into three repositories. The main firmware is located under *core-firmware*, while the supporting libraries are subdivided in to *core-common-lib* and *core-communication-lib*.

#### How do we *download* these repositories?
You can access all of the repositories via any git interface or download it directly from the website.

Make sure all of the following repositories are downloaded into the same folder. *For example (if all of the repositories are downloaded in a folder called Spark):*

```
D:\Spark\core-firmware
D:\Spark\core-common-lib
D:\Spark\core-communication-lib
```

*Method 1: Through the git command line interface.*  

Open up a terminal window, navigate to your destination directory and type the following commands:

(Make sure you have git installed on your machine!)

* `git clone https://github.com/spark/core-firmware.git`  
* `git clone https://github.com/spark/core-common-lib.git`  
* `git clone https://github.com/spark/core-communication-lib.git`  

*Method 2: Download the zipped files directly from the Spark's GitHub website*

* [core-firmware](https://github.com/spark/core-firmware/archive/master.zip)
* [core-common-lib](https://github.com/spark/core-common-lib/archive/master.zip)
* [core-communication-lib](https://github.com/spark/core-communication-lib/archive/master.zip)

#### How do we *build* these repositories?

Make sure you have downloaded and installed all the required dependencies as mentioned [previously.](#1-download-and-install-dependencies). Note, if you've downloaded or cloned these previously, you'll want to `git pull` or redownload all of them before proceeding.

Open up a terminal window, navigate to the build folder under core-firmware
(i.e. `cd core-firmware/build`) and type:

    make

This will build your main application (`core-firmware/src/application.cpp`) and required dependencies.

*For example:* `D:\Spark\core-firmware\build [master]> make`

##### Common Errors

* `arm-none-eabi-gcc` and other required gcc/arm binaries not in the PATH.
  Solution: Add the /bin folder to your $PATH (i.e. `export PATH="$PATH:<SOME_GCC_ARM_DIR>/bin`).
  Google "Add binary to PATH" for more details.

* You get `make: *** No targets specified and no makefile found.  Stop.`.
  Solution: `cd core-firmware/build`.

Please issue a pull request if you come across similar issues/fixes that trip you up.

### Navigating the code base

All of the repositories are sub divided into functional folders:

1. `/src` holds all the source code files
2. `/inc` holds all the header files
3. `/build` holds the makefile and is also the destination for the compiled `.bin` and `.hex` files.

## 3. Edit and Rebuild

Now that you have your hands on the entire Spark Core firmware, its time to start hacking!

### What to edit and what not to edit?

The main user code sits in the application.cpp file under core-firmware/src/ folder. Unless you know what you are doing, refrain yourself from making changes to any other files.

After you are done editing the files, you can rebuild the repository by running the `make` command in the `core-firmware/build` directory. If you have made changes to the other two repositories, make automatically determines which files need to be rebuilt and builds them for you.

## 4. Flash It!

Its now time to transfer your code to the Spark Core! You can always do this using the Over The Air update feature or, if you like wires, do it over the USB.

*Make sure you have the `dfu-util` command installed and available through the command line*

#### Steps:
1. Put you Core into the DFU mode by holding down the MODE button on the Core and then tapping on the RESET button once. Release the MODE button after you start to see the RGB LED flashing in yellow. It's easy to get this one wrong: Make sure you don't let go of the left button until you see flashing yellow, about 3 seconds after you release the right/RESET button. A flash of white then flashing green can happen when you get this wrong. You want flashing yellow.

2. Open up a terminal window on your computer and type this command to find out if the Core indeed being detected correctly. 

   `dfu-util -l`   
   you should get the following in return:
   ```
   Found DFU: [1d50:607f] devnum=0, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/20*001Ka,108*001Kg" 
   Found DFU: [1d50:607f] devnum=0, cfg=1, intf=0, alt=1, name="@SPI Flash : SST25x/0x00000000/512*04Kg"
   ```

   (Windows users will need to use the Zatig utility to replace the USB driver as described earlier)

3. Now, navigate to the build folder in your core-firmware repository and use the following command to transfer the *.bin* file into the Core.
   ```
   dfu-util -d 1d50:607f -a 0 -s 0x08005000:leave -D core-firmware.bin
   ```

   For example, this is how my terminal looks like:
   ```
D:\Spark\core-firmware\build [master]> dfu-util -d 1d50:607f -a 0 -s 0x08005000:leave -D core-firmware.bin
   ```
Upon successful transfer, the Core will automatically reset and start the running the program.

##### Common Errors
* As of 12/4/13, you will likely see `Error during download get_status` as the last line from 
the `dfu-util` command. You can ignore this message for now.  We're not sure what this error is all about.

* If you are having trouble with dfu-util, (like invalid dfuse address), try a newer version of dfu-util. v0.7 works well.

**Still having troubles?** Checkout our [resources page](https://www.spark.io/resources), hit us up on IRC, etc.

### CREDITS AND ATTRIBUTIONS

The Spark application team: Zachary Crockett, Satish Nair, Zach Supalla, David Middlecamp and Mohit Bhoite.

The core-firmware uses the GNU GCC toolchain for ARM Cortex-M processors, ARM's CMSIS libraries, TI's CC3000 host driver libraries, STM32 standard peripheral libraries and Arduino's implementation of Wiring.

### LICENSE

Unless stated elsewhere, file headers or otherwise, all files herein are licensed under an LGPLv3 license. For more information, please read the LICENSE file.

### CONTRIBUTE

Want to contribute to the Spark Core project? Follow [this link]() to find out how.

### CONNECT

Having problems or have awesome suggestions? Connect with us [here.](https://community.sparkdevices.com/)

### VERSION HISTORY

Latest Version: v1.0.0
