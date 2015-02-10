## ADC testing hardware requirements:

Connect a voltage divider circuit with equal resistor values(100K) as follows on Core:
```
3V3* <-------> 100K <-------> A5 <-------> 100K <-------> GND
```

Connect a jumper to DAC and A5 pins i.e. DAC-A5 lines should be shorted on Photon
```
DAC1 <-------> A5
```

## Flashing the wiring/adc_dac files to the core/photon

```
cd firmware/user
make v=1 TEST=wiring/adc_dac all program-dfu
```


