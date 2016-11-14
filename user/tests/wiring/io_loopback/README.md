## IO pins loopback test

Connect neighbour pair of IO pins together to check that digitalRead and
digitalWrite work as expected. See <io_loopback.cpp> for the pin pairs
for each platform.


## Flashing the wiring/io_loopback

### Raspberry Pi

```
export FIRMWARE=~/Programming/firmware
docker run --rm -it -v $FIRMWARE:/firmware -v $DIR/output:/output particle/buildpack-raspberrypi make APPDIR= TEST=wiring/io_loopback
particle flash my_pi $FIRMWARE/build/target/main/platform-31/io_loopback
```

