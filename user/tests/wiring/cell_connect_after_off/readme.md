The following illustrates the problem statement:
> Keep antenna disconnected for this test
1. With antenna disconnected, run `Particle.connect()`
2. Run `Cellular.off()`
3. After 60 sec, run `Particle.connect()` again, and verify that you see cellular AT traffic to turn on the modem or connect to the network. 