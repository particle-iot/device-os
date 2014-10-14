## ADC testing hardware requirements:

Connect a voltage divider circuit with equal resistor values(100K) as follows:
3V3* ------- 100K ------- A5 ------- 100K ------- GND

## SERIAL testing hardware requirements:

Connect a jumper to TX and RX pins i.e. TX-RX lines should be shorted

## Flashing the core-v1/testapp2 files to the core

```
cd firmware/main
make v=1 TEST=core-v1/testapp2 all program-dfu
```


