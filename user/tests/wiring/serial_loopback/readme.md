SERIAL testing hardware requirements
------------------------------------

Connect a jumper to TX and RX pins i.e. TX-RX lines should be shorted

```none
TX <-------> RX
```

Flashing the wiring/serial_loopback files
-------------------------------------------------------

```none
cd firmware/user
make v=1 TEST=wiring/serial_loopback all program-dfu
```
