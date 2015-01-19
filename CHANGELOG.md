## v0.4.0

### UNDER THE HOOD

 - Platform: hardware dependencies are factored out from wiring into a hardware abstraction layer
 - Repo: all 3 spark repos (core-common-lib, core-communication-lib, core-firmware) are combined into this repo. 
 - Modularization: factored common-lib into `platform`, `services` and `hal` modules. 
 - Modularization: factored core-firmware into `wiring` and `main` modules. 
 - Build system: fancy shmancy new build system - [build/readme.md](build/readme.md)

### ENHANCEMENTS

 - loop() iteration rate increased by 1000 times - from 200 Hz to over 200 kHz!
 - Compiler: Removed all warnings from the compile (and made warnings as errors) so compiler output is pristine!
 - Debugging: SWD Support, thanks to Elco Jacobs. [#337](https://github.com/spark/core-firmware/pull/337)

### BUGFIXES

- SOS calling `Spark.publish()` in `SEMI_AUTOMATIC`/`MANUAL` mode
- Subscriptions maintained when cloud disconnected. [#278](https://github.com/spark/firmware/issues/278)
- Fix for events with composite names. [#382](https://github.com/spark/firmware/issues/382)
- `WiFi.ready()` returning true after `WiFi.off()` in manual mode. [#378](https://github.com/spark/firmware/issues/378)

## v0.3.3

### FEATURES

 - Cloud: [Secure random seed](https://github.com/spark/core-communication-lib/pull/25). When the spark does a handshake with the cloud, it receives a random number that is set as a seed for `rand()` 
 - Wiring: Arduino-compatible `random()` and `randomSeed()` functions. [#289](https://github.com/spark/core-firmware/pull/289)
 - Wiring: Arduino-compatible functions like `isALpha()` and `toLowerCase()`. [#293](https://github.com/spark/core-firmware/pull/293)
 - Wiring: USB HID Mouse and Keyboard support using local build only. [#162](https://github.com/spark/firmware/pull/162)
 - Libraries: Flash update using serial(Serial, Serial1 or Serial2) port based on Ymodem protocol. [#250](https://github.com/spark/firmware/pull/250)

### ENHANCEMENTS

 - Wire: added missing Slave mode using DMA/Interrupts and updated Master mode using DMA. New APIs `Wire.setSpeed()` and `Wire.strechClock()`. [#284](https://github.com/spark/core-firmware/issues/284)
 - Sleep: `Spark.sleep()` supports wakeup on pin change. [#265](https://github.com/spark/core-firmware/issues/265)
 
### BUGFIXES

 - RGB: calling `RGB.brightness()` doesn't change the LED brightness immediately [#261](https://github.com/spark/core-firmware/issues/261)
 - Wiring: `pinMode()` `INPUT` and `OUTPUT` constants had reversed values compared to Arduino. [#282](https://github.com/spark/core-firmware/issues/282)
 - Wiring: compiler error using `HEX` with `String`. [#210](https://github.com/spark/core-firmware/pull/210)
 - System Mode: MANUAL mode breaks OTA udpate [#294](https://github.com/spark/core-firmware/issues/294)
 
 
