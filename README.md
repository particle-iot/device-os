# marvin

Spark firmware for the STM32 with CC3000

Development Environment Setup:

1. Install GCC (GNU Compiler Collection) for ARM Cortex processors: https://launchpad.net/gcc-arm-embedded

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

6. Follow the screenshots in the folder "eclipse_marvin_setup"(Steps: 1 to 30)
   PS: The screenshots are not available within the GIT project(probably need to be put in Dropbox).
   
7. Important Project Settings when building for various platforms/boards:

8. In Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Compiler -> Optimization,
   "CHECK" the "Function sections".
   "UNCHECK" the "Data sections".

9. For Flashing marvin.bin via DFU, follow the steps below:

   In Eclipse Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Linker -> General -> Script file (-T),
   Browse & select linker file : "linker_stm32f10x_md_dfu.ld"

   Uncomment the following line in platform_config.h to enable DFU based marvin build
   "#define DFU_BUILD_ENABLE"

10. Build marvin project using Eclipse

11. To build the project using command line option, cd to the "build" folder and run make.

12. Make sure the board is first flashed with the usb-dfu.hex file in the "usb-dfu" project using JTAG

13. Enter DFU mode by keeping the BUTTON pressed for > 1sec

14. Install open-source "dfu-util" on Windows, MAC or Linux by following the link below:
    http://dfu-util.gnumonks.org/index.html

15. Add "dfu-util" related bin files to PATH environment variable

16. List the currently attached DFU capable USB devices by running the following command on host:
    dfu-util -l

17. The STM32 boards in usb-dfu mode should be listed as follows:
    Found DFU: [0483:df11] devnum=0, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/12*001Ka,116*001Kg"
    Found DFU: [0483:df11] devnum=0, cfg=1, intf=0, alt=1, name="@SPI Flash : SST25x/0x00000000/512*04Kg"

18. cd to the marvin/Debug folder and type the below command to program the marvin application using dfu-util:
    dfu-util -d 0483:df11 -a 0 -s 0x0800A000:leave -D marvin.bin

19. To Enter Smart Config Mode, press the BUT once.
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
