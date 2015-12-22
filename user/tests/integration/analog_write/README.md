# `analogWrite`

## BOM

* Electron
* Logic Analyzer

## Setup

Just put the electron in a breadboard. Connect each channel of the logic Analyzer
to a PWM-capable pin. I only have an 8ch analyzer, so I did it in shifts. I did
the A/B side of the board first and then the C/D side of the board.

## Flash

* Put your Electron in DFU mode
* `cd firmware/main`
* `make clean` (optional)
* `time make all PLATFORM=electron APP=../tests/integration/analog_write/firmware DEBUG_BUILD=y DEBUG=1 PARTICLE_DEVELOP=1 program-dfu`

## Test

* I sampled for 2 seconds and looked at the waveforms to make sure they match.
