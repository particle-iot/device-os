## Spark Core Firmware

This is the Github repository for the firmware for the Spark Core.

### Who this is for

This README is for beta testers and early users of the Spark Core. After reading this README, you should be able to:
- Connect your Spark Core to the internet
- Send API calls to your Spark Core
- Write software for the Spark Core
- Reprogram the Spark Core over USB
- Design your own API calls (and call them)
- Review and edit the Spark libraries
- Submit changes to the Spark libraries
- *Advanced users only*: Reprogram the Spark Core over JTAG


### What's in the box

Congratulations on being the owner of a brand new Spark Core! Go ahead, open the box, and let's talk about what you see. Your box should include:
- *One Spark Core.* The reason you bought this thing. We'll dig in more here in a bit.
- *One breadboard.* A breadboard makes it easy to wire components to the Core without solder. Internally, the rows are electrically connected horizontally, and the "rails" along the edges are connected vertically. See (this website)[http://some-website-that-explains-breadboards] on breadboards for more information.
- *One USB cable.* The included USB cable is used for two things: powering the Spark Core (by connecting it to your computer, to a USB power brick, or to a USB battery pack) and reprogramming. In the long run, reprogramming the Core will generally be done wirelessly, but we have not yet implemented this functionality, so reprogramming must be done over USB.


### Let's talk about the Core

Take a look at the Spark Core. There are a few important things to be aware of on the board:
- *Buttons*. There are two buttons on the Core: the RESET button and the USER button. The RESET button will put the Core in a hard reset, effectively depowering and repowering the microcontroller. This is a good way to restart the application that you've downloaded onto the Core. The USER button serves three functions:
  - *Tap the USER button* to put the Core in _Smart Config_ mode to connect the Core to a new Wi-Fi network.
  - *Hold the USER button for two seconds* to put the Core in USB-DFU mode to reprogram it over USB.
- *LEDs*. There are three LEDs on the Core. The right-most LED lights up whenever the Core is powered. The other two LEDs display the current state of the internet connection, as follows:
  - Left off: No Wi-Fi connection
  - Left flashing: In _Smart Config_ mode
  - Left steady on: Connected to local Wi-Fi network
  - Right off: No Spark Cloud connection
  - Right flashing: Searching for Spark Cloud
  - Right steady on: Connected to Spark Cloud
- *Pins*. The Core has 24 pins that you can connect a circuit to. These pins are:
  - _RAW_: Connect an unregulated power source here with a voltage between 3.7V and 9V to power the Core. If you're powering the Core over USB, this pin should not be used.
  - _VCC_: This pin will output a regulated 3.3V power rail that can be used to power any components outside the Core. (Also, if you have your own 3.3V regulated power source, you can plug it in here to power the Core).
  - _VDDA_: This is a separate low-noise regulated 3.3V power rail designed for analog circuitry that may be susceptible to noise from the digital components. If you're using any sensitive analog sensors, power them from _VDDA_ instead of from _VCC_.
  - _!RST_: You can reset the Core (same as pressing the RESET button) by connecting this pin to GND.
  - _GND_: These pins are your ground pins.
  - _D0 to D7_: These are the bread and butter of the Spark Core: 8 GPIO (General Purpose Input/Output) pins. They're labeled "D" because they are "Digital" pins, meaning they can't read the values of analog sensors. Some of these pins have additional peripherals (SPI, JTAG, etc.) available, keep reading to find out more.
  - _A0 to A7_: These pins are 8 more GPIO pins, to bring the total count up to 16. These pins are just like D0 to D7, but they are "Analog" pins, which means they can read the values of analog sensors (technically speaking they have an ADC peripheral). As with the Digital pins, some of these pins have additional peripherals available.
  - _TX and RX_: These pins are for communicating over Serial/UART. TX represents the transmitting pin, and RX represents the receiving pin.


### Getting started

#### Step 1: Set up a simple LED circuit

(This section should explain how the user can wire up an LED and a resistor to D0 in order to control it over the Spark API)

#### Step 2: Connect your Spark Core to the internet

The Spark Core comes with software pre-installed that will connect the Core to the internet over your home Wi-Fi network and, once it's connected, let you control the Digital pins using the Spark Cloud API.

(This section should explain how the user can download the _Smart Config_ app to either their iOS or Android device, and the steps necessary to provision access to the user's local Wi-Fi network. I've included existing documentation as a starting point).

To Enter Smart Config Mode, press the BUT once.
    Both the LED should start toggling.
    Run the Smart Config App on an iPhone/itouch/Android device.
    Enter the following information:
    SSID: (Probably set automatically)
    Password:
    Gateway IP address: (Probably set automatically)
    Key: sparkdevices2013 (Hardcoded in Spark Hardware)
    Device Name: CC3000 (Hardcoded in Spark Hardware)
    When the device is connected to AP, LED2 should stop toggling and turn ON after few seconds.
    The App on phone should also display a connected successfully message.


#### Step 3: Make an API call

(This section should explain how a user can use the basic D0 to HIGH/LOW API call to turn on and off the LED connected to pin D0).


### Writing software for the Core

When using the Spark Core software package, you have access to the following libraries:
  - *Arduino/Wiring (mostly implemented).* Wiring is the "programming language" that Arduino uses, and is commonly described as the Arduino programming language because of this. We have ported over much of Wiring to our micro-controller (the STM32). See below for which functions have been ported and which have not yet been ported, but will be by the time we deliver the Core in September.
  - *ST Standard Peripheral Library.* ST, the manufacturers of the STM32, have a library called the ST Standard Peripheral Library that lets users write roughly the same code regardless of which ST microcontroller they are using. Experienced embedded systems designers may be most comfortable using these libraries. Please see (ST's documentation)[http://link-to-st-documentation] for more information on these functions.
  - *Low level GPIO.* For the daring, you can write low-level code by writing to the various registers included on the STM32 chip. Please see the (STM32 user manual)[http://link-to-stm32-user-manual] for more information.
  - *Messaging functions for Spark Cloud API.* Besides the basic Arduino/Wiring code, we've created our own functions for sending messages around on the internet. These functions are documented below.

#### Arduino/Wiring functions

(Each of the following functions should be a link to the appropriate documentation on the Arduino site. Also, I don't think I caught everything here, so let's finish fleshing this out)
Functions currently implemented:
`digitalWrite()`
`digitalRead()`
`analogWrite()`
`analogRead()`
`Serial.write()`
`Serial.read()`
`Serial.print()`
`Serial.println()`

Functions not yet implemented:
`SPI`
`I2C`
`Additional Serial functions`
`Keyboard`

There are two Serial ports on the Core: one over USB and one over the TX/RX pins on the Core. To communicate over USB, use Serial (i.e. `Serial.print()`); to communicate over the TX/RX pins, use Serial1 (i.e. `Serial1.print()`).

#### Spark Cloud API functions

We have created two additional functions for sending messages through the Spark Cloud: one for handling messages received by the Spark Core (`handleMessage()`) and one for sending messages up to the Spark Cloud (`sendMessage()`). We plan on creating other functions for common use cases (such as calling functions on the Core remotely, or reading variables off of the Core), but for now, this basic messaging protocol is a simple method to handle most of the things you'll want to do.

(Each of the following should provide clear documentation for how the function should be used, its limitations (i.e. number of characters in a message), and example code for how it might be used in a project).

`handleMessage()`

`sendMessage()`


### Reprogramming the Spark Core

#### Step 1: Install dependencies

(This section should explain how the user should install arm-none-eabi and dfu-util on Mac/Windows/Linux).

- Install GCC (GNU Compiler Collection) for ARM Cortex processors:
  https://launchpad.net/gcc-arm-embedded

- Install open-source "dfu-util" on Windows, MAC or Linux by following the link below:
  http://dfu-util.gnumonks.org/index.html

#### Step 2: Compile your code

(This section should explain how to compile the Spark Core project using arm-none-eabi and our own compile script. I've included the existing documentation here as a starting point)


2. Add path to environment variable.

3. Make sure you are able to run "make" from the terminal window.
   In Windows it needs to be explicitly installed.

4. Install Eclipse CDT

5. In Eclipse, go to (Help > Install New Software).
   click the add button, and insert the following text:

   Name: GNU ARM Eclipse Plug-in
   Location: http://gnuarmeclipse.sourceforge.net/updates

   Click OK and a component named "CDT GNU Cross Development Tools" will appear,
   check it, then click the Next button and follow the installation instructions.

6. Follow the screenshots in the folder "eclipse_core-firmware_setup"(Steps: 1 to 30)
   PS: The screenshots are not available within the GIT project(probably need to be put in Dropbox).
   
7. Important Project Settings when building for various platforms/boards:

8. In Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Compiler -> Optimization,
   "CHECK" the "Function sections".
   "UNCHECK" the "Data sections".

9. For Flashing core-firmware.bin via DFU, follow the steps below:

   In Eclipse Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Linker -> General -> Script file (-T),
   Browse & select linker file : "linker_stm32f10x_md_dfu.ld"

   Uncomment the following line in platform_config.h to enable DFU based core-firmware build
   "#define DFU_BUILD_ENABLE"

10. Build core-firmware project using Eclipse

11. Alternatively to build the project using command line option, cd to the "build" folder and run "make clean" followed by "make all".

## Step 3: Flash your code

(This section should explain how to flash the compiled code onto the Spark Core using dfu-util. I've included the existing documentation as a starting point)

12. Make sure the board is first flashed with the usb-dfu.hex file in the "usb-dfu" project using JTAG

13. Enter DFU mode by keeping the BUTTON pressed for > 1sec

15. Add "dfu-util" related bin files to PATH environment variable

16. List the currently attached DFU capable USB devices by running the following command on host:
    dfu-util -l

17. The STM32 boards in usb-dfu mode should be listed as follows:
    Found DFU: [0483:df11] devnum=0, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/12*001Ka,116*001Kg"
    Found DFU: [0483:df11] devnum=0, cfg=1, intf=0, alt=1, name="@SPI Flash : SST25x/0x00000000/512*04Kg"

18. cd to the core-firmware/Debug folder and type the below command to program the core-firmware application using dfu-util:
    dfu-util -d 0483:df11 -a 0 -s 0x0800C000:leave -D core-firmware.bin

#### Step 4: Test your code

(This section should explain how to use Serial-over-USB for debugging)


### Designing your own API calls

(This section should explain how to determine an abstract message that should be passed to/from the Core, how to include that message in the Core software application, and how to pass the message from the API. Sample code for both incoming and outgoing messages would be helpful).


### Review and edit the Spark libraries

(This section should explain what's included in the Spark libraries, including some basic description of the directory structure, in case people want to dig in and make edits).


### Submit changes to the Spark libraries

(This section should explain how to create a branch in git and push changes to that branch, and how to document the changes on Github, so folks who aren't familiar with git and Github can contribute to the project)


### *Advanced users only:* Reprogram the Spark Core over JTAG

(This section is low priority, and should explain how users can reprogram the Core over JTAG if they so desire. We should include lots of WARNINGS about installing code with DFU_ENABLE this way, since that can potentially brick the Core. We should also include a memory map so people know where to put different bits and pieces.)


### Troubleshooting

(This section should give users instructions for where to go when they run into trouble. Basically the options might include emailing us, messaging us on Google Hangouts/IRC, or creating a thread on our community site).