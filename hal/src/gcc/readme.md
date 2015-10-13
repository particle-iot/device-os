# Building

## Prereqs

GCC 4.9  - `brew isntall gcc49`
- clone boost and build
```
git clone --recursive http://github.com/boostorg/boost.git boost
export BOOST_ROOT=/path/to/boost-dir/boost
cd boost
./bootstrap.sh
./b2

export DYLD_LIBRARY_PATH=$(BOOST_ROOT)/stage/lib
```


alternatively (for windows):
```
sudo ./bjam --install --link=static --runtime-link=static --layout=tagged --with-system threading=single architecture=x86
```

## Building
```
git checkout develop
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
|


