
THe firmware and bootloader are built by running the `make` command. 

Running

```
make
```

in the top-level directory creates the bootloader and firmware binaries, which are output to subdirectories of the `build/target/` directory.

The top-level make is mainly a convenience to build `bootloader` and `main` projects. It
supports these targets: 
- `clean`  - force the next build to be a full rebuild, and
- `all` (default), build the artefact. 

By default, the Core is the target platform. To build for the Photon, run

```
make PLATFORM=photon
```


## Build Components

The build system is organized as a number of build components. Each build component
exists in it's own directory, with it's own makefiles and is responsible for
build the artifacts that make up that component.

These are the primary components that produce executable code for a device:

- bootloader
- main
- modules

The other projects are libraries used by these main projects. 

When building firmware, it's a good idea to build from `main`, since this offers
additional features compared to building in the root directory.


# Quick Start

## Targets

- `all`: the default target - builds the artefact for the project
- `clean`: deletes all artefacts so the next build runs from a clean state
- 'all program-dfu': (not bootloader) - builds and flashes the executable to a device via dfu
- 'all st-flash': flashes the executable to a device via the st-link `st-flash` utility


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
    can be outside of the firmware repo, allowing 3rd party applications to be built.
    See `USER_MAKEFILE`.
- `TEST` builds the test application stored in `user/tests/$(TEST)`.
- `USER_MAKEFILE`: when `APPDIR` is used this specifies the location of the makefile
    to include, relative to `APPDIR`. The default is `build.mk`. 

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
| photon   | 6           |
| P1       | 8           |

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



## Building a User Application

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

(Please note this is an experimental feature and may be removed in future releases.)

When using `APP` or `APPDIR` to build custom application sources, the build system
by default will build any `.c` and `.cpp` files found in the given directory
and it's subdirectories. You can override this and customize the build process by adding the file
`build.mk` to the root of the application sources. The file should be a valid gnu make file.

To customize the build, append values to these variables:

- `CSRC`, `CPPSRC`: the c and cpp files in the build which are compiled and linked, e.g.
```
SRC += $(call target_files,,*.c)
CPPSRC += $(call target_files,,*.cpp)
```
To add all files in the application directory and subdirectories.

- `INCLUDE_DIRS`: the include path. Paths are relative to the APPDIR folder. 
- `LIB_DIRS`: the library search path
- `LIBS`: libraries to link (found in the library search path). Library names are given without the `lib` prefix and `.a` suffix.
- `LIB_DEPS`: full path of additional library files to include.

To use a different customization file other than `build.mk`, define `USER_MAKEFILE` to point to
your custom build file, relative to the application sources. 


## Integrated application.cpp with firmware

In previous versions of the make system, the application code was integrated with the firmware code at `src/application.cpp`.
This mode of building is still supported, however the location has changed to: `user/src/application.cpp`.

To build the default application sources, simply run `make`

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
installed and in your PATH (Windows), and the core put in DFU mode (flashing yellow).

```
cd main
make program-dfu
```

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

After compiling, you should see ca 3000 bytes reduction in statically allocated
RAM and, ca 35k reduction in flash use.
