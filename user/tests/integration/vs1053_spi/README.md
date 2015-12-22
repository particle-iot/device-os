# VS1053 SPI*

\* and SPI1 and SPI1(alt) tests

Using an Adafruit SPI-based [VS1053 breakout board](https://www.adafruit.com/products/1381), we're going to play a sound file in a loop. This breakout has two SPI devices,
a micro SD card reader as well as the VS1053 chip.

If you don't have all the parts on hand, it might be kind of a hard test to duplicate
quickly.

## BOM

* Electron
* [Adafruit VS1053](https://www.adafruit.com/products/1381)
* breadboard
* jumper wires
* MicroSD card compatible with the breakout board
* headphones and/or an amplified speaker with a headphone jack ([similar to this](http://www.amazon.com/X-Mini-XAM4-B-Portable-Capsule-Speaker/dp/B001UEBN42/)).

## Setup

* Solder on the headers on the VS1053 board.
* Format the SD card.
* Drop a short mp3 file (< 5 seconds in duration) onto the SD card named `track001.mp3`
  * I used [this one](http://www.freesfx.co.uk/download/?type=mp3&id=6368) from http://www.freesfx.co.uk. Any < 5s MP3 file will do, though.

### Shared pin setup

This pins are shared across all examples. Go ahead and set them up now and they
won't change.

| VS1053 Breakout | Electron |
| :--- | :-- |
| VCC  | VIN |
| GND  | GND |
| DREQ | A1  |
| RST  | D4  |
| CS   | D5  |
| SDCS | A0  |
| XDCS | D6  |

| VS1053 Breakout | 1/8" headphone jack |
| :--------- | :------------ |
| LOUT       | left pin *    |
| ROUT       | right pin *   |
| AGND       | center pin    |
\* When the headphone jack port is facing toward you, pins down

### SPI

| VS1053 Breakout | Electron |
| :--- | :- |
| MISO | A4 |
| MOSI | A5 |
| SCLK | A3 |

Make sure `#define SPI_INTERFACE` is set to `SPI1` in `Adafruit_VS1053.cpp:27`
and `Sd2Card.cpp:59`.

### SPI1

| VS1053 Breakout | Electron |
| :--- | :- |
| MISO | D3 |
| MOSI | D2 |
| SCLK | D4 |

Make sure `#define SPI_INTERFACE` is set to `SPI1` in `Adafruit_VS1053.cpp:27`
and `Sd2Card.cpp:59`.

### SPI2

| VS1053 Breakout | Electron |
| :--- | :- |
| MISO | C2 |
| MOSI | C1 |
| SCLK | C3 |

Make sure `#define SPI_INTERFACE` is set to `SPI1` in `Adafruit_VS1053.cpp:27`
and `Sd2Card.cpp:59`.

TODO: Set compile flag to alternate... Not sure how

## Flash

* Put your Electron in DFU mode
* `cd firmware/main`
* `make clean` (optional)
* `time make all PLATFORM=electron APP=../tests/integration/vs1053_spi/firmware DEBUG_BUILD=y DEBUG=1 PARTICLE_DEVELOP=1 program-dfu`

## Test

* As soon as DFU completes, Electron will reboot.
* Watch serial terminal
    * `screen /dev/tty.usbmodem141141 9600` or `screen /dev/tty.usb<hit TAB key> 9600`
* Watch for debugging output to fly by on serial terminal
* Once RGB status LED starts to breathe cyan, look for output like this:
  <br>`S1053 found.`
  <br>`SD initialized.`
  <br>`Playing track.`
