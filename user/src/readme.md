Testing app + Teacup firmware

- On startup it runs the teacup app. 
- Testing mode is available only from listening/setup mode. For a freshly flashed device, this will enter listening mode automatically. 
- Once in listening mode, send these bytes over Serial1 to enable testing mode:
 
```
0xe1, 0x63, 0x57, 0x3f, 0xe7, 0x87, 0xc2, 0xa6, 0x85, 0x20, 0xa5, 0x6c, 0xe3, 0x04, 0x9e, 0xa0
```

On receiving these bytes the LED will change color (but will still continue blinking) indicating
the Tester app is active. The device will remain in testing mode until it is reset/powered off.

## Tester Commands

### CONNECT:&lt;ssid&gt;:&lt;pwd&gt;;

Connects to the given WiFi AP. After a few seconds you'll see `DHCP` printed a few bazillion times to the console.

### RESET:;

Reset the device and perform a factory reset.

### DFU:;

Restarts the device and places it in DFU mode.

### LOCK:;

Locks the bootloader region.

### UNLOCK:;

Unlocks the bootloader region.

### REBOOT:;

Resets the device.

### INFO:; 

Requests device info. Print a block of data to the console like this:

```
DeviceID: 220033000947333531303339
Serial: 854B1F
MAC: 44:39:c4:ee:b5
SSID: beer palace
RSSI: -34
```

### CLEAR:;

Clears the wifi credentials if there are any on the device. This is a good command to have as the last step, if a factory reset isn't being done.

