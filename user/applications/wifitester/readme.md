# Testing app

The testing app is available out of the box when compiled as the user application.
(`wifitester.bin`). You can use it right away by sending commands over Serial or Serial1.

The manufacturing/testing app is also built into the system firmware on the Photon
and later devices.

- Testing mode is available from listening/setup mode. For a freshly flashed device, this will enter listening mode automatically.
- Once in listening mode, send these bytes over Serial1 to enable testing mode:

```
0xe1, 0x63, 0x57, 0x3f, 0xe7, 0x87, 0xc2, 0xa6, 0x85, 0x20, 0xa5, 0x6c, 0xe3, 0x04, 0x9e, 0xa0
```

On receiving these bytes the LED will change color (but will still continue blinking) indicating
the Tester app is active. The device will remain in testing mode until it is reset/powered off.




## Tester Commands

The commands have the format:

&lt;commant-name&gt;:&lt;optional-parameters&gt;;

The colon and semi-colon are important parts of the command and shouldn't be omitted, or the command will not be recognized.


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

### WIFI_SCAN:;

Performs a wifi scan and prints the results of the scan, showing the SSID and RSSI for each AP discovered.

### SET_PIN:pin:state;

Sets the state of a given pin:

- pin: D0..D7, A0..A7 or ALL to set all pins to the same state
- state: HIGH or LOW

Examples:

```
SET_PIN:D0:HIGH;
SET_PIN:A5:LOW;
SET_PIN:ALL:LOW;
```

### SET_PRODUCT:id;

Sets the product ID. The ID is persisted.

Example

```
SET_PROUDCT:8;
```
Sets the product ID to 8.


### ANT:;

Performs a test of the internal and external antennae by selecting first the internal then the external
antenna and performing a wifi scan.

### ANT:antenna;

Toggles the RF switch to select the named antenna.

The `antenna` parameter can be one of:
- AUTO: auto select
- INTERNAL: the internal ceramic antenna
- EXTERNAL: the uFL antenna

The antenna parameter can be abbreviated down to a single letter if desired.

```
ANT:INT;
Selected antenna INTERNAL

ANT:E;
Selected antenna EXTERNAL
```





