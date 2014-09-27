This root project folder contains the top-level makefile:

> One Makefile to rule them all, One Makefile to find them; One ring to bring 
> them all and in the darkness build them.

To build all projects simply run

```make```

This creates build artefacts for all projects under the `build/target` directory.

## clean build

The top-level makefile also supports the `clean` goal. It calls clean on all
submodules before deleting the `build\target` directory.

E.g.

```make clean all```

## Controlling Verbosity

By default the makefile is quiet - the only output is when an .elf file is produced to
show the size of the flash and RAM memory regions. To produce more verbose output, define
the `v` (verbose) variable, like this:

```
make v=1
```

## Building individual modules

Each module can be built on its own by executing the makefile in the module's directory:

```
cd main
make
```

## Specifying the target Product ID

By default, the build system targets the stock Spark Core (Product ID 0). If
your product has been given a different product ID, you should pass this on the
command line to specifically target your product. For example:

```
make SPARK_PRODUCT_ID=2
```

Would build the bootloader and firmware for product ID 2.


## Building a User Application

To build a new application, first create a subdirectory under `main/applications`. 
You'll find the Tinker app is already there. Let's say we want to create a new
app, which we'll call "myapp":

```
mkdir main/applications/myapp
```

Then add the files needed for your application to that directory. These can be named freely,
but should end with `.cpp`. For example, you might create these files:

```
myapp.cpp
halper.cpp
helper.h
```

You can also add header files - the application subdirectory is part of the include path.

To build this application, change directory to the `build` directory and run

```
make APP=myapp
```

This will build your application with the resulting `.bin` file available in
`build/target/main/prod-0/applications/myapp/myapp.bin`. If you prefer the output to appear in the build directory
you can define the `TARGET_DIR` variable:

```
make APP=myapp TARGET_DIR=
```

This will place `tinker.bin` (and the other output files) in `build/target/main/prod-0'.

Finally, to continue naming the output file `core-firmware.bin`, define `TARGET_FILE` 
like this:

```
make APP=myapp TARGET_FILE=core-firmware
```

This will build the firmware with output as `core-firmware.bin` in `build/target/main/prod-0'.


### Integrated application with firmware

In previous versions of the make system, the application code was integrated with the firmware code.
This mode of building is still supported.

To build firmware with the application source in `main/src/application.cpp` simply run `make`:

```
make
```

This will then build the firmware as before with the user code in `application.cpp`

## Build directory changes

The build system uses a `out of source` directory for all built artefacts. The
directory is `build/target`. In previous versions of the build system, artefacts
were placed under a local `build` folder. If you would prefer to maintain this style of
working, you can create a symlink from `build/target/main/prod-0` to `main/build`.
Then after building main, the artefacts will be available in the `build` subdirectory
as before.



