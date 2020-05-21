# Introduction

All firmware described below is designed to allow AT command PASS-THROUGH for Gen 2 2G/3G/Cat-M1 devices.  All firmware will either connect to the cellular tower by default, or it will explicitly not connect to the tower.  Only the versions that connect to the tower automatically also have a manually operated "connect to Particle Cloud" command (typically not used for cellular certification testing though).

# Programming

### Flash system firmware

```
particle flash --usb electron-system-part1@1.5.2.bin
particle flash --usb electron-system-part2@1.5.2.bin
particle flash --usb electron-system-part3@1.5.2.bin
```

### Select an appropriate application:

### 2G/3G devices

```
particle flash --usb at-pass-through-2g3g-conn@1.5.2.bin (connects by default)
particle flash --usb at-pass-through-2g3g-no-conn@1.5.2.bin (does not connect by default)
```

### Cat-M1 devices

```
particle flash --usb at-pass-through-catm1-conn@1.5.2.bin (connects by default)
particle flash --usb at-pass-through-catm1-no-conn@1.5.2.bin (does not connect by default)
```


# THERE ARE 4 DIFFERENT APPLICATION VERSIONS DESCRIBED BELOW:

## 1. at-pass-through-catm1-no-conn 

### (does not connect to tower by default, cat-m1 only)

- On initial power up, the RGB LED will turn WHITE, RED, GREEN, BLUE, dim WHITE just as a test.
- This firmware defaults to powering the modem ON, ECHO ON, ASSIST ON, DISconnected.  RGB LED should be BLUE after the modem powers on.
- There is an AT pass through available on USB serial.
- You can run :con; to register to EPS (connect to the tower).  This also establishes the data connection on LTE.  The LED will turn GREEN.
- The firmware will not try to connect to Particle or send any traffic.
- They can choose not to run :con; for the Particle SIM and run their own AT commands to make any connection they wish.
- The LED will turn RED if one of the commands below fails.
- Full debugging output of all AT commands is available on the TX pin at 115200 baud.
- See other commands and LED states below.

#### Commands supported:

```
[:on;     ] turn the cellular modem ON (default), LED=BLUE
[:off;    ] turn the cellular modem OFF, LED=WHITE
[:echo1;  ] AT command echo ON (default)
[:echo0;  ] AT command echo OFF
[:assist1;] turns on ESC, BACKSPACE and UP ARROW keyboard assistance (default)
[:assist0;] turns off ESC, BACKSPACE and UP ARROW keyboard assistance
[:con;    ] Start the EPS connection process, LED=GREEN (EPS)
[:dis;    ] Network disconnect and stop polling the EPS status (default), LED=BLUE
[:cloud1; ] Connect to the Particle Cloud, LED=CYAN
[:cloud0; ] Disconnect from the Particle Cloud, LED=GREEN
[:prof1;  ] Set MNO profile +UMNOPROF to (AT&T)
[:prof0;  ] Set MNO profile +UMNOPROF to (default)
[:help;   ] show this help menu
```

## 2. at-pass-through-catm1-conn

### (connects to tower by default, cat-m1 only)

- Similar operation as at-pass-through-catm1-no-conn
- Will register to EPS (connect to the tower) by default. RGB LED will be GREEN.

#### Commands supported:

```
[:on;     ] turn the cellular modem ON (default), LED=BLUE
[:off;    ] turn the cellular modem OFF, LED=WHITE
[:echo1;  ] AT command echo ON (default)
[:echo0;  ] AT command echo OFF
[:assist1;] turns on ESC, BACKSPACE and UP ARROW keyboard assistance (default)
[:assist0;] turns off ESC, BACKSPACE and UP ARROW keyboard assistance
[:con;    ] Start the EPS connection process, LED=GREEN (EPS)
[:dis;    ] Network disconnect and stop polling the EPS status (default), LED=BLUE
[:cloud1; ] Connect to the Particle Cloud, LED=CYAN
[:cloud0; ] Disconnect from the Particle Cloud, LED=GREEN
[:prof1;  ] Set MNO profile +UMNOPROF to (AT&T)
[:prof0;  ] Set MNO profile +UMNOPROF to (default)
[:help;   ] show this help menu
```

## 3. at-pass-through-2g3g-no-conn

### (does not connect to tower by default, 2G/3G only)

- On initial power up, the RGB LED will turn WHITE, RED, GREEN, BLUE, dim WHITE just as a test.
- This firmware defaults to powering the modem ON, ECHO ON, ASSIST ON, DISconnected.  RGB LED should be BLUE after the modem powers on.
- There is an AT pass through available on USB serial.
- You can run :con; to register to GMS/GPRS (connect to the tower).  This also establishes the data connection.  The LED will turn GREEN.
- The firmware will not try to connect to Particle or send any traffic.
- They can choose not to run :con; for the Particle SIM and run their own AT commands to make any connection they wish.
- The LED will turn RED if one of the commands below fails.
- Full debugging output of all AT commands is available on the TX pin at 115200 baud.
- See other commands and LED states below.

#### Commands supported:

```
[:on;     ] turn the cellular modem ON (default), LED=BLUE
[:off;    ] turn the cellular modem OFF, LED=WHITE
[:echo1;  ] AT command echo ON (default)
[:echo0;  ] AT command echo OFF
[:assist1;] turns on ESC, BACKSPACE and UP ARROW keyboard assistance (default)
[:assist0;] turns off ESC, BACKSPACE and UP ARROW keyboard assistance
[:con;    ] Start the GSM/GPRS connection process, LED=GREEN (GSM & GPRS)
[:dis;    ] Network disconnect and stop polling the GSM/GPRS status (default), LED=BLUE
[:help;   ] show this help menu
```

## 4. at-pass-through-2g3g-conn

### (connects to tower by default, 2G/3G only)

- Similar operation as at-pass-through-2g3g-no-conn
- Will register to GSM/GPRS (connect to the tower) by default. RGB LED will be GREEN.

#### Commands supported:

```
[:on;     ] turn the cellular modem ON (default), LED=BLUE
[:off;    ] turn the cellular modem OFF, LED=WHITE
[:echo1;  ] AT command echo ON (default)
[:echo0;  ] AT command echo OFF
[:assist1;] turns on ESC, BACKSPACE and UP ARROW keyboard assistance (default)
[:assist0;] turns off ESC, BACKSPACE and UP ARROW keyboard assistance
[:con;    ] Start the GSM/GPRS connection process, LED=GREEN (GSM & GPRS)
[:dis;    ] Network disconnect and stop polling the GSM/GPRS status (default), LED=BLUE
[:help;   ] show this help menu
```

---------------------------------------------------------------





