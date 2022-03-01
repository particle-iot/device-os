## Serial testing hardware requirements

Connect a jumper to TX and RX pins i.e. TX-RX lines should be shorted.
```
TX <-------> RX
```

## Flashing the wiring/serial_loopback2 test application

```
modules $ make TEST=wiring/serial_loopback2 PLATFORM=<platform> clean all program-dfu
```
