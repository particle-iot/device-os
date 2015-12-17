# I2C test with the SparkFun Photon IMU shield

This uses the code from the [SparkFun library](https://github.com/sparkfun/Photon_IMU_Shield/commit/ccb384fa8d421d557d80189f20acbe4f6e707faf).
It spits out readings over `Serial` every 250ms-ish.

## Prerequisites

- Photon / Electron
- [SparkFun 9DoF IMU breakout shield](https://www.sparkfun.com/products/13629)
- USB connection to computer for serial connection
- firmware repo checked out and local build env working

## Device Setup

- Plug device into shield the right way.

## Build instructions

- ensure your device is in dfu mode and claimed to your currently active particle-cli account
- put the device in DFU mode
- from the root of firmware `cd main`
- `make all PLATFORM=xxx APP=../tests/integration/sparkfun_i2c_imu/firmware program-dfu`* to flash the cloud test app to your device

\* You may also want/need to add `DEBUG_BUILD=y DEBUG=1 PARTICLE_DEVELOP=1` to the above make command.
