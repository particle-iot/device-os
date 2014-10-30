How to use the Makefile 
-----------------------

Building a User Application
---------------------------

To build a new application, first create a subdirectory under `applications`. 
You'll find the Tinker app is already there. Let's say we want to create a new
app, which we'll call "myapp":

```
mkdir applications/myapp
```

Then add the files needed for your application to that directory. These can be named freely,
but should end with `.cpp`. For example, you might create these files:

```
myapp.cpp
helper.cpp
helper.h
```

You can also add header files - the application subdirectory is part of the include path.

To build this application, change directory to the `build` directory and run

```
make APP=myapp
```

This will build your application with the resulting `.bin` file available in
`build/applications/myapp/myapp.bin`. If you prefer the output to appear in the build directory
you can define the `TARGETDIR` variable:

```
make APP=myapp TARGETDIR=
```

This will place `tinker.bin` (and the other output files) directly in the build directory.

Finally, to continue naming the output file `core-firmware.bin`, define `TARGET` 
like this:

```
make APP=myapp TARGET=core-firmware
```

This will build the firmare with output as `core-firmware.bin` in the `build` directory.


Integrated application with firmware
------------------------------------

In previous versions of the make system, the application code was integrated with the firmware code.
This mode of building is still supported.

To build firmware with the application source in `src/application.cpp` simply run `make`:

```
make
```

This will then build the firmware as before, with the result placed in `build\core-firmware.bin`.

Specifying custom toolchain
---------------------------

Custom compiler prefix can be used via an environment variable `GCC_PREFIX`.

For example when you have installed a custom toolchain under
`/opt/gcc-arm-embedded-bin` you can use invoke make using that toolchain
like this:

```
GCC_PREFIX="/opt/gcc-arm-embedded-bin/bin/arm-none-eabi-" make
```
