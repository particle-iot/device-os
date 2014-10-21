## v0.3.4

### FEATURES

- Local Build: Specify custom toolchain with `GCC_PREFIX` environment variable ([firmware](https://github.com/spark/firmware/pull/328), [core-common-lib](https://github.com/spark/core-common-lib/pull/39), [core-communication-lib](https://github.com/spark/core-communication-lib/pull/29))

### ENHANCEMENTS

- Wiring: More efficient and reliable `print(String)` (fix issue [#281](https://github.com/spark/firmware/issues/281)) [#305](https://github.com/spark/firmware/pull/305)
- DFU: Add DFU suffix to .bin file [#323](https://github.com/spark/firmware/pull/323)

### BUGFIXES

- I2C: Use I2C polling mode by default [#322](https://github.com/spark/firmware/pull/322)
- Listening Mode: Fix hard fault when Wi-Fi is off [#320](https://github.com/spark/firmware/pull/320)
- LED Interaction: Fix breathing blue that should be blinking green [#315](https://github.com/spark/firmware/pull/315)


## v0.3.3

### FEATURES

 - Cloud: [Secure random seed](https://github.com/spark/core-communication-lib/pull/25). When the spark does a handshake with the cloud, it receives a random number that is set as a seed for `rand()` 
 - Wiring: Arduino-compatible `random()` and `randomSeed()` functions. [#289](https://github.com/spark/core-firmware/pull/289)
 - Wiring: Arduino-compatible functions like `isAlpha()` and `toLowerCase()`. [#293](https://github.com/spark/core-firmware/pull/293)

### ENHANCEMENTS

 - Wire: added missing Slave mode using DMA/Interrupts and updated Master mode using DMA. New APIs `Wire.setSpeed()` and `Wire.strechClock()`. [#284](https://github.com/spark/core-firmware/issues/284)
 - Sleep: `Spark.sleep()` supports wakeup on pin change. [#265](https://github.com/spark/core-firmware/issues/265)
 
### BUGFIXES

 - RGB: calling `RGB.brightness()` doesn't change the LED brightness immediately [#261](https://github.com/spark/core-firmware/issues/261)
 - Wiring: `pinMode()` `INPUT` and `OUTPUT` constants had reversed values compared to Arduino. [#282](https://github.com/spark/core-firmware/issues/282)
 - Wiring: compiler error using `HEX` with `String`. [#210](https://github.com/spark/core-firmware/pull/210)
 - System Mode: MANUAL mode breaks OTA update [#294](https://github.com/spark/core-firmware/issues/294)

## pre v0.3.3 versions

See https://github.com/spark/core-firmware/releases
