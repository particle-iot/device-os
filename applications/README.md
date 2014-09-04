# Local build setup

Put whatever subdirectories you like in the applications/ and libraries/ directories, they won't conflict with the upstream core-firmware git repo. 

Using as an example a BMP085 sensor application with supporting library, the directory structure should be:

```
core-firmware/libraries/bmp085/
    bmp085.cpp
    bmp085.h

core-firmware/applications/bmp085_test/
   bmp085_test.cpp
   Makefile
```

The bmp085_test/Makefile file should contain any C++ source files used in your application or libraries:

```
CPPSRC += applications/bmp085_test/bmp085_test.cpp     # the main application
CPPSRC += libraries/bmp085/bmp085.cpp                  # the sensor library
```

As per the cloud IDE, you should include libraries with their directory name, for example in bmp085_test.cpp:

```
 #include "bmp085/bmp085.h"
```

When you want to compile a specific application, call ```make``` and pass in the application's name, which should be the same as its directory name, for example:

```
make APP=bmp085_test
```
Or to simply compile the default Tinker application, omit the APP flag:

```
make clean all
```

Note that you need to pass in the APP name (except for the default Tinker) when calling all make targets, for example:

```
make APP=bmp085_test program-dfu
```
