## Serial testing hardware requirements

Wiring

```none
                        74xx125
           10k          ___ ____
  (3V3)--/\/\/\--(A2)--|1  u  14|-- (3V3)
  (TX)-----------------|2       |
  (RX)-----------------|3       |
                       |4       |
                       |5       |
                       |6       |
  (GND)----------------|7      8|
                        ^^^^^^^^
```

## Flashing the wiring/serial_loopback2 test application

```
modules $ make TEST=wiring/serial_loopback2 PLATFORM=<platform> clean all program-dfu
```
