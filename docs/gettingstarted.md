# Getting Started

1. [Download and Install Dependencies](dependencies.md#1-download-and-install-dependencies)
2. [Download and Build Repositories](#2-download-and-build-repositories)
3. [Edit and Rebuild](#3-edit-and-rebuild)
4. [Flash It!](#4-flash-it)

## 2. Download and Build Repositories

The entire Particle firmware is contained in this repository.

There are two ways to download
- through the git command line interface
- download the zipped file from the github website

We recommend the first approach, since it makes keeping up to date with new releases
much simpler.

**Method 1:** Through the git command line interface.

Open up a terminal window, navigate to your destination directory and type the following commands:

(Make sure you have git installed on your machine!)

* `git clone --recursive https://github.com/particle-iot/firmware.git`

**Method 2:** Download the zipped files directly from the GitHub website

* [`develop` branch firmware](https://github.com/particle-iot/firmware/archive/develop.zip)

#### How do we *build* these repositories?

Make sure you have downloaded and installed all the required dependencies as mentioned [previously](dependencies.md#1-download-and-install-dependencies).
Note, if you've downloaded or cloned these previously, you'll want to `git pull` or redownload all of them before proceeding.

Open up a terminal window, and switch branch to 'release/stable'

    git checkout release/stable

Navigate to the `modules` folder under firmware
(i.e. `cd firmware/modules`) and type:

    make clean all PLATFORM=photon -s program-dfu

This will clean build the system firmware and the default main application (`firmware/user/src/application.cpp`) which contains Tinker, but you may overwrite this with your own application and add any required dependencies. The `-s` silences the verbose output, so be patient while it builds.  If your device is in DFU mode, it will then download the 3 binaries one at a time.  For more custom application location solutions, see the [makefile documentation](build.md) and learn how to use the `APP=myapp` option.

The [makefile documentation](build.md) describes the build options supported and how to target platforms other than the Core (i.e., Photon, P1, Electron, etc..)

##### Common Errors

* `arm-none-eabi-gcc` and other required gcc/arm binaries not in the PATH.
  Solution: Add the /bin folder to your $PATH (i.e. `export PATH="$PATH:<SOME_GCC_ARM_DIR>/bin`).
  Google "Add binary to PATH" for more details.

* You get `make: *** No targets specified and no makefile found.  Stop.`
  Solution: `cd firmware/main`

* GCC does not know what an .ino file is, so your application is empty.
  Solution: rename yourapp.ino file to yourapp.cpp

* GCC shows errors for Particle specific commands.
  Solution: #include "application.h"

Please issue a pull request if you come across similar issues/fixes that trip you up.

### Navigating the code base

All of the top-level directories are sub divided into functional folders that are
the various libraries that make up the firmware.

| library | description |
|---:|:---|
| **platform** | bare-metal services, the lowest layer in the system |
| **bootloader** | the bootloader, with sources for each platform |
| **hal** | the Hardware Abstraction Layer interface and an implementation for each supported platform |
| **services** | platform neutral services and macros (LED control, debug macros, static assertions) |
| **communication** | implements the protocol between the device and the cloud |
| **dynalib** | framework for producing dynamically linked libraries |
| **system** | the system firmware (Networking, firmware updates.) |
| **wiring** | the Wiring API |
| **user** | contains the default application code (Tinker) and your own applications |
| **main** | top-level project to build the firmware for a device |
| **modules** | dynamically linked modules for the Photon/PØ/P1 |

Within each library, the structure is

1. `/src` holds all the source code files
2. `/inc` holds all the header files

The compiled `.bin` and `.hex` files are output to a subdirectory of `build/target/`.
The exact location is given in the final compiler output. (It depends upon the platform and on what is being built.)

## 3. Edit and Rebuild

Now that you have your hands on the firmware, its time to start hacking!

### What to edit and what not to edit?

The main user code sits in the application.cpp file under firmware/user/src/ folder. Unless you know what you are doing, refrain yourself from making changes to any other files.

After you are done editing the files, you can rebuild the repository by running the `make` command in the `firmware/main/` directory.
If you have made changes to any of the other directories, make automatically determines which files need to be rebuilt and builds them for you.

## 4. Flash It!

Its now time to transfer your code to your Particle device! You can always do this using the Over the Air update feature or, if you like wires, do it over the USB.

*Make sure you have the `dfu-util` command installed and available through the command line*

#### Steps:
1. Put your device into the DFU mode by holding down the mode/setup button on the device and then tapping on the RESET button once. Release the MODE/setup button after you start to see the RGB LED flashing in yellow.
It's easy to get this one wrong: Make sure you don't let go of the MODE/SETUP button until you see flashing yellow, about 3 seconds after you release the RESET button.
A flash of white then flashing green can happen when you get this wrong. You want flashing yellow.

2. Open up a terminal window on your computer and type this command to find out if the device indeed being detected correctly.

   `dfu-util -l`
   you should get something similar to this in return:
   ```
   Found DFU: [1d50:607f] devnum=0, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/20*001Ka,108*001Kg"
   Found DFU: [1d50:607f] devnum=0, cfg=1, intf=0, alt=1, name="@SPI Flash : SST25x/0x00000000/512*04Kg"
   ```

   - Windows users will need to use the Zatig utility to replace the USB driver as described earlier
   - Linux users who encounter someting like: `dfu-util: Cannot open DFU device 2b04:d00a` can try `sudo dfu-util -l` or if you use the Particle CLI command `particle flash --usb` or `particle update` it will add the device udev rules automatically for you.


3. Now, from the `main/` folder in your firmware repository and use the following command to transfer the *.bin* file into the device.
   ```
   make program-dfu
   ```

Upon successful transfer, the device will automatically reset and start the running the program.

##### Common Errors
* As of 12/4/13, you will likely see `Error during download get_status` as the last line from
the `dfu-util` command. You can ignore this message for now.  We're not sure what this error is all about.

* If you are having trouble with dfu-util, (like invalid dfuse address), try a newer version of dfu-util. v0.7 works well.

**Still having troubles?** Checkout our [resources page](https://www.particle.io/resources), hit us up on IRC, etc.
