Particle Bootloader
===================

### Building

```
device-os/bootloader $ make clean all -s PLATFORM=boron
```

### Flashing

After building the bootloader, it can be flashed to the device OTA (Over-the-Air) using Particle CLI.

```none
particle flash <devicename> bootloader.bin
```
It can also be flashed over the wire.

```none
particle flash --serial bootloader.bin
```

How does the bootloader work?
=============================

The bootloader is designed to simply transfer firmware binaries to and from various locations that include internal flash memory, the external SPI flash memory, and the USB port.

The bootloader is the first thing that is executed upon a system reset.

---

### Program Flow

* Initialize the system
* Load system flags
* Check system flags and select a mode (one of the following):
      * OTA Successful
      * OTA Fail
      * Factory reset mode
      * Enter DFU Mode
* Check for MODE button status and set the appropriate flags
* Take decisions based on the status of the flags set/read previously
      * If OTA was successful -> Load FW from OTA location
      * If Factory reset mode was selected -> Load FW from FAC address
      * If OTA failed -> Load FW from BKP address
* Check if a FW is available. If yes, jump to that address
* If not, enter the DFU mode