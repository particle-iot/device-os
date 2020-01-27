SERIAL testing hardware requirements
------------------------------------

Connect a jumper to TX and RX pins i.e. TX-RX lines should be shorted on Core/Photon

```none
TX <-------> RX
```

Connect a jumper to D0 and D1 pins i.e. D0-D1 lines should be shorted on Core only

```none
D0 <-------> D1
```

Flashing the wiring/serial_loopback files to the photon
-------------------------------------------------------

```none
cd firmware/user
make v=1 TEST=wiring/serial_loopback all program-dfu
```
