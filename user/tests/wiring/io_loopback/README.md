## IO pins loopback test

Connect neighbour pair of IO pins together to check that digitalRead and
digitalWrite work as expected. See <io_loopback.cpp> for the pin pairs
for each platform.


## Flashing the wiring/io_loopback

### Raspberry Pi
```
docker run --rm -v firmware:/firmware -v output:/output particle/buildpack-raspberrypi -e MAKE_ARGS="TEST=wiring/io_loopback"
```


