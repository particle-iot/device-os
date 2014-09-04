## v3.3

### FEATURES

 - Cloud: Secure random seed. When the spark does a handshake with the cloud, it receives a random number that is set as a seed for `rand()`  
 - Wiring: Arduino-compatible `random()` and `randomSeed()` functions. #289

### ENHANCEMENTS

 - Wire: added missing Slave mode using DMA/Interrupts and updated Master mode using DMA. New APIs `Wire.setSpeed()` and `Wire.strechClock()`. #284
 - Sleep: `Spark.sleep()` supports wakeup on pin change. #265
 
### BUGFIXES

 - RGB: calling `RGB.brightness()` doesn't change the LED brightness immediately #261
 - Wiring: `pinMode()` `INPUT` and `OUTPUT` constants had reversed values compared to Arduino. #282
 
