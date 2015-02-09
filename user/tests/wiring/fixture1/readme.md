## ADC testing hardware requirements:

Connect a voltage divider circuit with equal resistor values(100K) as follows on Core:
```
3V3* <-------> 100K <-------> A5 <-------> 100K <-------> GND
```

Connect a jumper to DAC and A5 pins i.e. DAC-A5 lines should be shorted on Photon
```
DAC1 <-------> A5
```

## SERIAL testing hardware requirements:

Connect a jumper to TX and RX pins i.e. TX-RX lines should be shorted on Core/Photon
```
TX <-------> RX
```

Connect a jumper to D0 and D1 pins i.e. D0-D1 lines should be shorted on Core only
```
D0 <-------> D1
```

## Flashing the wiring/fixture1 files to the core/photon

```
cd firmware/user
make v=1 TEST=wiring/fixture1 all program-dfu
```


