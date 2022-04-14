ADC testing hardware requirements
---------------------------------

Connect a voltage divider circuit with equal resistor values(100K) as follows on Photon:
Connect a jumper to DAC and A5 pins i.e. DAC-A5 lines should be shorted.
Connect a jumper to DAC2/A3 and A1 pins i.e. A3-A1 lines should be shorted.

```none
DAC1      <-------> A5
DAC2 (A3) <-------> A1

```

Flashing the wiring/adc_dac files to the photon
-----------------------------------------------

```none
cd firmware/main
make v=1 TEST=wiring/adc_dac all program-dfu
```
