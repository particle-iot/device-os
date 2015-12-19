# SPI test with the SparkFun Photon IMU shield

This uses the code from the [SparkFun library](https://github.com/sparkfun/Photon_IMU_Shield/commit/ccb384fa8d421d557d80189f20acbe4f6e707faf).
It spits out readings over `Serial` every 250ms-ish.

## Prerequisites

- Photon / Electron
- [SparkFun 9DoF IMU breakout shield](https://www.sparkfun.com/products/13629)
- USB connection to computer for serial connection
- firmware repo checked out and local build env working

## Device Setup

### Testing `Wire` (D0/D1)
Plug device into shield the correct way will test D0/D1

### Testing `Wire1` (C4/C5) on Electron

Plug device into breadboard and jumper these connections:

| IMU Shield     | Device      |
| :------------- | :---------- |
| D0             | C4          |
| D1             | C5          |
| 3v3            | 3v3         |
| GND            | GND         |

Change SparkFunLSM9DS1.cpp:30's `#define WIRE` to `#define WIRE1`

## Build instructions

- ensure your device is in dfu mode and claimed to your currently active
  particle-cli account
- put the device in DFU mode
- get into firmware/main
- `make all PLATFORM=xxx APP=../tests/integration/sparkfun_i2c_imu/firmware program-dfu`*
  to flash the cloud test app to your device.

\* You may also want/need to add `DEBUG_BUILD=y DEBUG=1 PARTICLE_DEVELOP=1` to
the above make command depending on your needs.

## Run test

After flash, attach to the device's serial output

- screen /dev/tty.usb<tab>
- minicom
- whatever you normally use to talk to the device via serial

You should see telemetry output every 250ms or so and when you move the board
around, you should see the values change.
