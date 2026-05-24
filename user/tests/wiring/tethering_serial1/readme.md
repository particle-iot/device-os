Tethering Serial1 testing hardware requirements
-----------------------------------------------

Wiring

- Particle Debugger used as Serial to USB adapter
- Connect all 5 lines (RX, TX, CTS, RTS, GND)
- msom (gen 4) - can use som eval board
- b5som (gen 3) - use m-hat hardware

```
  Device            Particle Debugger

  (TX)--------------(RX)
  (RX)--------------(TX)
  (CTS)-------------(RTS)
  (RTS)-------------(CTS)
  (GND)-------------(GND)

```
