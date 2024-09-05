SERIAL testing hardware requirements
------------------------------------

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

Flashing the wiring/serial_loopback files
-------------------------------------------------------

```none
cd firmware/user
make v=1 TEST=wiring/serial_loopback all program-dfu
```
