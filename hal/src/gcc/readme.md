# Building

## Prereqs

### Boost

On OSX:

```
 brew install gcc49 wget
 source ./ci/install_boost.sh
 ./ci/build_boost.sh
 export DYLD_LIBRARY_PATH=$BOOST_ROOT/stage/lib
```

On Windows:

- Ensure gcc is in your path (e.g. from MinGW). Tested on version 4.9.3. 
- Download [boost 1.61.0](http://heanet.dl.sourceforge.net/project/boost/boost/1.61.0/boost_1_61_0.zip) from sourceforge and xtract to a folder.
- Define `BOOST_ROOT` to point to the folder containing boost, e.g.
```
set BOOST_ROOT=c:\dev\boost_1_61_0
```

Then build the boost libraries

```
cd %BOOST_ROOT%
bootstrap.bat gcc
bjam --install --layout=tagged --with-system --with-program_options --with-random --with-thread toolset=gcc
```

## Building the Virtual Device

```
cd main
make -s PRODUCT_ID=3
```
The resulting executable is placed in `build/target/main/platform-3/main`.



# Device Configuration

## Configuration Sources

The device can be configured using any of these sources:

- command line arguments: to see the list of command line arguments supported, run with `--help`
- configuration file: the file `vdev.conf` is read from the current directory if it exists.
- environment variables: the names above are turned into environment variables by making them uppercase, and prefixing with VDEV_. For example,
  the device id is configured with the environment variable VDEV_DEVICE_ID




## Configuration Values

The virtual device is configured using keys and values.

| Name                       | Description                                           |
| -------------------------- | ----------------------------------------------------- |
| device_id                  | the unique ID for this device, maximum 12 digits      |
| device_key                 | the file containing the device's private key          |
| server_key                 | the file containing the cloud public key              |
| protocol                   | `tcp` or `udp`                                            |


## Troubleshooting

### Build

Duplicate sections that are different sizes. 
- the same templates are linked into different boost libraries, producing different compiler output. This can be due to different compiler flags. Use `-n -a` flags with `bjam` to see the tool invocations and compare flags. 





