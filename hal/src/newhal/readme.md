This folder contains the sources to bootstrap development of a new HAL implementation.

## Bootstrapping a new HAL:

- test build an empty HAL: `make PLATFORM_ID=60000` this creates empty elf files for the bootloader and main. (They are empty because the linker scripts are empty, so no sections are output.)
- get a new Platform ID from Particle (or assign the next free one yourself temporarily until one is assigned to you). Platform IDs are listed in `build/platform-id.mk`
- duplicate all the parts in platform-id.mk referring to `newhal` and rename to your product
- duplicate this directory (`hal/src/newhal/`), and rename to match `PLATFORM_NAME` as defined for your product (in platform-id.mk)
- duplicate `platform/MCU/newhal-mcu` to a new folder and rename to match the `PLATFORM_MCU` value for your product (in product-id.mk)
- duplicate `build/arm/linker/linker_newhalcpu.ld` and rename, substituting newhalcpu for the value of `STM32_DEVICE` for your product.
- duplicate `build/arm/startup/linker_newhalcpu.S` and rename, substituting newhalcpu for the value of `STM32_DEVICE` for your product.
- create a new symbolic reference for your new platform in `firmware/platform/shared/platforms.h` that uses the same PLATFORM_ID you created earlier (e.g. `#define PLATFORM_ELECTRON_PRODUCTION 10`).
- add your new symbolic reference to `platform_config.h` to keep this from throwing an error, optionally add it in the appropriate places and configure all of the pin/port/interrupt mapping.
- test build - `make v=1 PLATFORM_ID=<your platform id> clean all` should now build the empty bootloader and firmware.

### Development typically starts off without a bootloader, focusing initially on building the firmware by porting the HAL to your target device.

>Your hal implementation directory is  `hal/src/<yourproduct>`.

#### These are the outline steps to implement a new HAL:

- add memory regions, linker sections (interrupt vector table, .text, .rodata, .data, .bss, etc..), entry function etc. to the `linker.ld` script located `here hal/src/<yourproduct>`. Alternatively you may rename this file, and point to it from the `INCLUDE.mk` file located in the same place. Make sure to also include the directory `hal/src/<yourproduct>` in the commented out `LDFLAGS += -L/some/directory` in `INCLUDE.mk`. If you are unfamiliar with the terminology in a linker script, this is a good reference on the [linker command language](https://sourceware.org/binutils/docs/ld/Scripts.html#Scripts).
- Do not move on until: 1) all ASSERT commands at the end of the linker script pass.  2) functions may be missing from the `hal/src/template` files, which will generate `undefined references` until you add them in their respective template files.  3) the `main` build target completes and starts to have a filesize as follows:
```
mv ../build/target/main/platform-10/main.bin.pre_crc ../build/target/main/platform-10/main.bin
   text	   data	    bss	    dec	    hex	filename
  13096	    224	   2156	  15476	   3c74	../build/target/main/platform-10/main.elf
```
- copy individual files from `hal/src/template` to your hal implementation folder. It's ok to rename the file from .c to .cpp and vice-versa.
- implement the function bodies in the chosen file

#### Suggested porting order to get a device connected to the cloud:

- core_hal      (initialization, sleep modes)
- deviceid_hal  (the unique ID send to the cloud)
- usb_hal       (for Serial output)
- timer_hal
- delay_hal
- wlan_hal      (smartconfig is optional)
- inet_hal
- socket_hal    (server sockets are not used to communicate with the cloud)
- ota_flash_hal (to support OTA updates)

#### The remainder of the files are for dealing with peripherals - from the perspective of the core system, these are all optional and are only used from user code:

- adc_hal
- dac_hal
- eeprom_hal
- gpio_hal
- i2c_hal
- interrupts_hal
- pinmap_hal
- pwm_hal
- rng_hal
- rtc_hal
- servo_hal
- spi_hal
- tone_hal
- usart_hal
