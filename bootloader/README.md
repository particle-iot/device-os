Particle STM32 Bootloader
=========================

Photon/Electron
---------------

### OTA

After building the bootloader, it can be flashed to the device Over-the-Air using Particle CLI.

```none
particle flash <devicename> bootloader.bin
```

This takes care of unlocking the protected memory regions, flashing the bootloader, and reprotecting the memory regions.

### Using the Programmer Shield

- ensure that the environment variable `OPENOCD_HOME` points to the directory containing OpenOCD on your computer.
- connect the programmer shield and start a telnet session on port 3333 as described [here](https://github.com/spark/shields/tree/master/photon-shields/programmer-shield)
- send the openOCD commands via telnet

  ```none
  reset halt
  flash protect 0 0 off
  ```

- in the bootloader directory run

  ```none
  make PLATFORM=photon all program-openocd
  ```

- finally send the telnet command

  ```none
  flash protect 0 0 on
  ```
