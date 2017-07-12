## Setup

- python 3 with pyserial
- device 
   - with TX/RX (Serial1) connected to a Serial to USB bridge
   - flashed with 0.5.4 or >= 0.7.0-rc.2 for Electron >= 0.4.9 for Photon
   - [optional] connect USBSerial to the computer to allow automatically entering listening mode

## Execution

- Find the device that corresponds to the Serial1 port. On OSX be sure to use the `calling unit` device, e.g. `/dev/cu.usbserial-XXXX`

- If connected to USBSerial as well, find the device in `/dev/*` that corresponds to that. Otherwise, put theParticle device under test in listening mode. 

```
python test.py <Serial1-USB-Port> [USBSerial]
```

The test will enable the test module and send the INFO command. The test succeeds if the device correctly responds. 

