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

- ensure your device is in dfu mode and claimed to your currently active
  particle-cli account
- put the device in DFU mode
- get into firmware/main
- `make all PLATFORM=xxx APP=../tests/integration/sparkfun_i2c_imu/firmware program-dfu`*
  to flash the cloud test app to your device.

\* You may also want/need to add `DEBUG_BUILD=y DEBUG=1 PARTICLE_DEVELOP=1` to
the above make command depending on your needs.


## Notes

By default, this is testing I2C on D0 (SDA)/D1 (SCL) using `Wire`. Electron also has
C4 (SDA)/C5 (SCL) using `Wire1`. In order to test C4/C5, you have to take the Electron out of
the shield and hook up power (3v3 pin and GND) as well as jumper the Electron's C4 to D0 on the
shield, and Electron's C5 -> Shield's D1.
