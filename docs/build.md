
# The Build System

## Quick Start

Running

```
make
```
For the core, or

```
make PLATFORM=photon
```
for the Photon, in the top-level directory creates the bootloader and firmware binaries for your device, which are output to subdirectories of the `build/target/` directory.

The top-level make is mainly a convenience to build `bootloader` and `main` projects. It
supports these targets:
- `clean`  - force the next build to be a full rebuild, and
- `all` (default), build the artefact.

The [Recipes and Tips](#recipes-and-tips) section describes the most frequently used commands. The remaining sections describe the build system in detail.

## Build Components

The build system is organized as a number of build components. Each build component
exists in it's own directory, with it's own makefiles and is responsible for
build the artifacts that make up that component.

These are the primary components that produce executable code for a device:

- bootloader
- main (builds application firmware)
- modules (builds system+application firmware)

The other projects are libraries used by these primary projects.

When building firmware, it's a good idea to build from `main`, since this offers
additional features compared to building in the root directory, such as `program-dfu` to flash
the produced firmware to the device.

## Updating System Firmware (Photon)

When building locally on the photon from the develop branch, it is necessary
to update the system firmware to the latest version:

- put the Photon in DFU mode
- `cd modules`
- `make PLATFORM=photon clean all program-dfu`
- You can optionally add `APP`/`APPDIR`/`TEST` values to the command above to build a specific application as you would when building `main`.

This will flash the latest system modules and the application to your device.

A key indicator that this is necessary is that the Photon doesn't run your application
after flashing, due to a version mismatch. The onboard LED will breathe magenta
to indicate Safe Mode when the application firmware isn't run.

# Overview

## Targets

- `all`: the default target - builds the artefact for the project
- `clean`: deletes all artefacts so the next build runs from a clean state
- `all program-dfu`: (not bootloader) - builds and flashes the executable to a device via dfu
- `all st-flash`: flashes the executable to a device via the st-link `st-flash` utility


## Variables

`make` accepts variable definitions as part of the command invocation

- `v` - verbose - set to 1 to trigger verbose output
- `PLATFORM`/`PLATFORM_ID`: specifies the target platform, either as a name or as an ID.
- `PRODUCT_ID`: specifies the target product ID.
- `PRODUCT_FIRMWARE_VERSION`: specifies the firmware version that is sent to the cloud.
    Value from 0 to 65535.
- `GCC_PREFIX`: a prefix added to the ARM toolchain. Allows custom locations to be specified if
    the ARM tools are not in the path.

When building `main` or `modules`:

- `APP`: builds the application stored in `user/applications/$(APP)`. (The default is to build
    the application code in `user/src`
- `APPDIR`: builds the application located in $(APPDIR). The directory specified
    should be outside of the firmware repo working directory, allowing 3rd party applications to be built.
    See `USER_MAKEFILE`.
- `APPLIBS`: directories containing external firmware libraries. See [External Libraries](#external_libs)
- `TEST` builds the test application stored in `user/tests/$(TEST)`.
- `USER_MAKEFILE`: when `APPDIR` is used this specifies the location of the makefile
    to include, relative to `APPDIR`. The default is `build.mk`.
- `DEBUG_BUILD` described in [debugging](debugging.md)

When building `main`:

- `TARGET_NAME`: sets the base name of the artefact file produced. E.g. setting
    `TARGET_NAME=whereyou` would produce the target named `whereyou.bin` The default
    is the value of `APP`.
- `TARGET_DIR`: sets the directory where the target files are placed relative to
    the current directory.

## Platform name/IDs

The Platform ID describes the target platform.
If you are targeting the Spark Core, you can skip this section. A list of supported
platform IDs are listed in [platform-id.mk]((../build/platform-id.mk). The most
common are listed here:

| Name     | PLATFORM_ID |
|----------|:-----------:|
| core     | 0           |
| gcc      | 3           |
| photon   | 6           |
| p1       | 8           |
| electron | 10          |

The platform is specified on the command line as

```
PLATFORM_ID=<id>
```

or as

```
PLATFORM=name
```

For example

```
make PLATFORM=photon
```
Would build the firmware for the Photon / P0.

To avoid repeatedly specifying the platform on the command line, it can be set
as an environment variable.

Linux/OS X:

```
export PLATFORM=photon
```

Windows

```
set PLATFORM=photon
```

In the commands that follow, we avoid listing the PLATFORM explicitly to keep
the examples concise.


## Clean Build

```
make clean
```

This clears all output files from the build so that all sources are recompiled.


## Specifying custom toolchain location

Custom compiler prefix can be used via an environment variable `GCC_PREFIX`.

For example when you have installed a custom toolchain under
`/opt/gcc-arm-embedded-bin` you can use invoke make using that toolchain
like this:

```
GCC_PREFIX="/opt/gcc-arm-embedded-bin/bin/arm-none-eabi-" make
```

The default value of `GCC_PREFIX` is `arm-none-eabi`, which uses the ARM
version of the GCC toolchain, assumed to be in the path.

Alternatively, a path for the tools can be specified separately as `GCC_ARM_PATH`,
which, if specified should end with a directory separator, e.g.

```
GCC_ARM_PATH="/opt/gcc-arm-embedded-bin/bin/" make
```

## Controlling Verbosity

By default the makefile is quiet - the only output is when an .elf file is produced to
show the size of the flash and RAM memory regions. To produce more verbose output, define
the `v` (verbose) variable, like this:

```
make v=1
```

## Building individual modules

The top-level makefile builds all modules. Each module can be built on its own
by executing the makefile in the module's directory. The make also builds any dependencies.

For example, executing

```
cd main
make
```

Will build the main firmware, and all the modules the main firmware depends on.


## Product ID

By default, the build system targets the Spark Core (Product ID 0). If
your product has been assigned product ID, you should pass this on the
command line to specifically target your product. For example:

```
make PRODUCT_ID=2
```

Builds the firmware for product ID 2.

Note that this method works only for the Core. On later platforms, the PRODUCT ID and version
is specified in your application code via the macros:

```
PRODUCT_ID(id);
```

and

```
PRODUCT_VERSION(version)
```


## Building an Application

To build a new application, first create a subdirectory under `user/applications/`.
You'll find the Tinker app is already there. Let's say we want to create a new
app, which we'll call `myapp/`

```
mkdir user/applications/myapp
```

Then add the files needed for your application to that directory. These can be named freely,
but should end with `.cpp`. For example, you might create these files:

```
myapplication.cpp
mylibrary.cpp
mylibrary.h
```

You can also add header files - your application subdirectory is on the include path.

To build this application, change directory to  `main` directory and run

```
make APP=myapp
```

This will build your application with the resulting `.bin` file available in
`build/target/main/platform-0/applications/myapp/myapp.bin`.

### Including Libraries in your Application

To include libraries in your application, copy or symblink the library sources
into your application folder.

To importing libraries from the WebIDE:
 - rename the `firmware` folder to the same name as the library
 - remove the examples folder

The library should then compile successfully



## Changing the Target Directory

If you prefer the output to appear somewhere else than in the `build/` directory
you can define the `TARGET_DIR` variable:

```
make APP=myapp TARGET_DIR=my/custom/output
```

This will place `main.bin` (and the other output files) in `my/custom/output` relative to the current directory.
The directory is created if it doesn't exist.


## Changing the Target File name

It's also possible to specify the name of the output file, e.g. to revert to the
old naming convention of `core-firmware.bin`, set `TARGET_FILE`
like this:

```
make APP=myapp TARGET_FILE=core-firmware
```

This will build the firmware with output as `core-firmware.bin` in `build/target/main/platform-0/applications/myapp`.

These can of course also be combined like so:

```
make APP=myapp TARGET_DIR=myfolder TARGET_FILE=core-firmware
```

Which will produce `myfolder/core-firmware.elf`


## Compiling an application outside the firmware source

If you prefer to separate your application code from the firmware code,
the build system supports this, via the `APPDIR` parameter.

```
make APPDIR=/path/to/application/source [TARGET_DIR=/path/to/applications/output] [TARGET_FILE=basename]
```

Parameters:

- `APPDIR`: The relative or full path to the directory containing the user application
- `TARGET_DIR`: the directory where the build output should go. If not defined,
    output files willb e placed under a `target` directory of the application sources.
- `TARGET_FILE`: the basename of the files created. If not defined,
defaults to the name of the application sources directory.

## Custom makefile

When using `APP` or `APPDIR` to build custom application sources, the build system
by default will build any `.c` and `.cpp` files found in the given directory
and it's subdirectories. You can override this and customize the build process by adding the file
a makefile to the root of the application sources.

The makefile should be placed in the root of the application folder. The default name for the file is:

- when building with `APP=myapp` the default name is `myapp.mk`
- when building with `APPDIR=` the default name is `build.mk`

The file should be a valid gnu make file.

To customize the build, append values to these variables:

- `CSRC`, `CPPSRC`: the c and cpp files in the build which are compiled and linked, e.g.
```
CSRC += $(call target_files,,*.c)
CPPSRC += $(call target_files,,*.cpp)
```
To add all files in the application directory and subdirectories.

- `INCLUDE_DIRS`: the include path. Paths are relative to the APPDIR folder.
- `LIB_DIRS`: the library search path
- `LIBS`: libraries to link (found in the library search path). Library names are given without the `lib` prefix and `.a` suffix.
- `LIB_DEPS`: full path of additional library files to include.

To use a different name/location for customization makefile file other than `build.mk`, define `USER_MAKEFILE` to point to
your custom build file. The value of `USER_MAKEFILE` is the location of your custom makefile relative to the application sources.

<a name='external_libs'>
## External Libraries
</a>

_Note that this is preliminary support for external libraries to bring some feature parity with Build. Over the coming weeks, full support for libraries will be added._

External Particle libraries can be compiled and linked with firmware. To add one or more external libraries:

1. download the library sources store it in a directory outside the firmware, e.g.`/particle/libs/neopixel` for the neopixel library. 

2. remove the `examples` directory if it exists
```
cd /particle/libs/neopixel
rm -rf firmware/examples
```

3. Rename `firmware` to be the same as the library name. 
```
mv firmware neopixel
```
This is so that includes like `#include "neopixel/neopixel.h"` will function correctly. 

4. Add the APPLIBS variable to the make command which lists the directories contianing libraries to use. 
```
make APPDIR=/particle/apps/myapp APPLIBS=/particle/libs/neopixel
```


## Default application.cpp integrated with firmware

In previous versions of the make system, the application code was integrated with the firmware code at `src/application.cpp`.
This mode of building is still supported, however the location has changed to: `user/src/application.cpp`.

To build the default application sources, just run `make`

```
make
```


## Platform Specific vs Platform Agnostic builds

Currently the low level hardware specific details are abstracted away in the HAL (Hardware Abstraction Layer) implementation.
By default the makefile will build for the Spark Core platform which will allow you to add direct hardware calls in your application firmware.
You should however try to make use of the HAL functions and methods instead of making direct hardware calls, which will ensure your code is more future proof!
To build the firmware as platform agnostic, first run `make clean`, then simply include `SPARK_NO_PLATFORM=y` in the make command.
This is also a great way to find all of the places in your code that make hardware specific calls, as they should generate an error when building as platform agnostic.

```
make APP=myapp SPARK_NO_PLATFORM=y
```

## Build Output Directory

The build system uses an `out of source` directory for all built artifacts. The
directory is `build/target/`. In previous versions of the build system, artifacts
were placed under a local `build` folder. If you would prefer to maintain this style of
working, you can create a symlink from `build/target/main/platform-0/` to `main/build/`.
Then after building `main`, the artifacts will be available in the `build/` subdirectory
as before.


## Flashing the firmware to the device via DFU

The `program-dfu` target can be used when building from `main/` to flash
the compiled `.bin` file to the device using dfu-util. For this to work, `dfu-util` should be
installed and in your PATH (Windows), and the device put in DFU mode (flashing yellow).

```
cd main
make program-dfu
```

### Enabling DFU Mode automatically

Normally, the device requires physical button presses to enter DFU mode. The build
also supports automatic DFU mode, where the device will automatically enter DFU
mode as part of running the `program-dfu` target. To enable this, define the environment variable
`PARTICLE_SERIAL_DEV` to point to the name of the serial device. E.g.

```
PARTICLE_SERIAL_DEV=/dev/tty.usbmodem12345 make all program-dfu
```

the device will then automatically enter DFU mode and flash the firmware.

(Tested on OS X. Should work on other platforms that provide the `stty` command.)


## Flashing the firmware to the device via ST-Link

The `st-flash` target can be used to flash all executable code (bootloader, main and modules)
to the device. The flash uses the `st-flash` tool, which should be in your system path.

# Debugging

To enable JTAG debugging, add this to the command line:

```
USE_SWD_JTAG=y
```

and perform a clean build.

To enable SWD debugging only (freeing up 2 pins) add:

```
USE_SWD=y
```

and perform a clean build. For more details on SWD-only debugging
see https://github.com/spark/firmware/pull/337

## Compilation without Cloud Support

[Core only]

To release more resources for applications that don't use the cloud, add
SPARK_CLOUD=n to the make command line. This requires a clean build.

After compiling, you should see a 3000 bytes reduction in statically allocated RAM and 35k reduction in flash use.


## Building the `develop` branch

Before the 0.4.0 firmware was released, we recommended the develop branch for early adopters to obtain the code. This is still fine for early adopters, and people that want the bleeding edge, although please keep in mind the code is untested and unreleased.

Pre-releases are available in `release/vx.x.x-rc.x` branches.  Default released firmware is available as `release/vx.x.x`, which is also then duplicated to `release/stable` and `master` branches.


## Recipes and Tips

- The variables passed to make can also be provided as environment variables,
so you avoid having to type them out for each build. The environment variable value can be overridden
by passing the variable on the command line.
- `PLATFORM` set in the environment if you mainly build for one platform, e.g. the Photon.

### Photon

Here are some common recipes when working with the photon. Note that `PLATFORM=photon` doesn't need to be present if you have `PLATFORM=photon` already defined in your environment.

```
# Complete rebuild and DFU flash of latest system and application firmware
firmware/modules$ make clean all program-dfu PLATFORM=photon

# Incremental build and flash of latest system and application firmware
firmware/modules$ make all program-dfu PLATFORM=photon

# Build system and application for use with debugger (Programmer Shield)
# APP/APPDIR can also be specified here to build the non-default application
firmware/modules$ make clean all program-dfu PLATFORM=photon USE_SWD_JTAG=y

# Incremental build and flash user application.cpp only (note the directory)
firmware/main$ make all program-dfu PLATFORM=photon

# Build an external application
firmware/modules$ make all PLATFORM=photon APPDIR=~/my_app
```

For system firmware developers:

```
# Rebuild and flash the primary unit test application
firmware/main$ make clean all program-dfu TEST=wiring/no_fixture PLATFORM=photon

# Build the compilation test (don't flash on device)
firmware/main$ make TEST=wiring/api PLATFORM=photon
```
