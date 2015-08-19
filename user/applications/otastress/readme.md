
Application to stress-test OTA updates.

## Quick Start

- flash system and application firmware to the device
- reset stats with `particle call <device> reset`
- run `./otastress.sh <device> <file>` , e.g. point to a system-part1.bin file
- this will repeatedly flash the file to the device and collect stats on successes/errors.
- to see the stats, connect via serial. stats are printed at the start/end of each OTA update. The `print` function can also be called when not performing an OTA to write stats to serial.

`