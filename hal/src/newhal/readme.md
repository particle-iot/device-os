This folder contains the sources to bootstrap development of a new HAL implementation.

### Bootstrapping a new HAL:

- test build an empty HAL: `make PLATFORM_ID=60000` this creates empty elf files for the bootloader and main. (They are empty because the linker scripts are empty, so no sections are output.)
- get a new product ID from Particle (or assign the next free one yourself temporarily until one is assigned to you). Product IDs are listed in `build/platform-id.mk`
- duplicate all the parts in platform-id.mk referring to `newhal` and rename to your product
- duplicate this directory (`hal/src/newhal/`), and rename to match `PLATFORM_NAME` as defined for your product (in platform-id.mk)
- duplicate `platform/MCU/newhal-mcu` to a new folder and rename to match the `PLATFORM_MCU` value for your product (in product-id.mk)
- duplicate `build/arm/linker/linker_newhalcpu.ld` and rename, substituting newhalcpu for the value of `STM32_DEVICE` for your product.
- duplicate `build/arm/startup/linker_newhalcpu.S` and rename, substituting newhalcpu for the value of `STM32_DEVICE` for your product.
- test build - `make v=1 PLATFORM_ID=<your platform id> clean all` should now build the empty bootloader and firmware.

Development typically starts off without a bootloader, focusing initially
on building the firmware by porting the HAL to your target device.

Your hal implementation directory is  `hal/src/<yourproduct>`.

These are the outline steps to implement a new HAL:
- add memory regions, linker sections, entry  function etc. to the `linker.ld` script 
- copy individual files from `hal/src/template` to your hal implementation folder. It's ok to rename the file from .c to .cpp and vice-versa.
- implement the function bodies in the chosen file

Suggested porting order to get a device connected to the cloud:

- core_hal      (initialization, sleep modes)
- deviceid_hal  (the unique ID send to the cloud)
- usb_hal       (for Serial output)
- timer_hal
- delay_hal
- wlan_hal      (smartconfig is optional)
- inet_hal
- socket_hal    (server sockets are not used to communicate with the cloud)
- ota_flash_hal (to support OTA updates)


The remainder of the files are for dealing with peripherals - from the perspective
of the core system, these are all optional and are only used from user code:
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

