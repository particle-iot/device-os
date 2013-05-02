# dfu

Spark USB-DFU firmware for the STM32

Development Environment Setup:

1. Copy .cproject and .project files from "marvin" into "dfu" folder.

2. Search and replace "marvin" with "dfu" in the above 2 files.

3. In Eclipse, select File -> Import -> Existing Projects into Workspace -> Select root directry: -> Browse and select "dfu" folder -> Finish.

4. In Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Compiler -> Optimization, select Optimization level as "Optimize size(-Os)"

5. Build the project and load the dfu.hex or dfu.bin file via JTAG.

6. The device should appear as "STM Device in DFU Mode" on the Host platform.

7. Use of LEDs and BUTTON:
Both LED1 and LED2 should toggle simultaneously for 250ms indicating that DFU mode has been entered.
Press BUT just once to exit DFU mode and enter User Application mode. If no application project is loaded, DFU mode is entered by default.
Press BUT for > 1sec during reset to enter DFU mode.

8. Install open-source "dfu-util" on Windows, MAC or Linux by following the link below:
http://dfu-util.gnumonks.org/index.html

9. Add "dfu-util" related bin files to PATH environment variable

10. List the currently attached DFU capable USB devices by running the following command on host:
dfu-util -l

The Core, Debug or H103 boards in DFU mode should be listed as follows:
Found DFU: [0483:df11] devnum=0, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/12*001Ka,116*001Kg"

11. For Flashing marvin.bin using DFU, follow the steps below in Eclipse - marvin project:
* Get the latest marvin code from Github
* In Eclipse Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Linker -> General -> Script file (-T),
Browse & select linker file : "linker_stm32f10x_md_dfu.ld"
* Uncomment the following line in platform_config.h to enable DFU based marvin build
#define DFU_BUILD_ENABLE

12. Build the marvin project for DFU usage.

13. cd to the marvin/Debug folder and type the below command to program the marvin application via DFU:
dfu-util -d 0483:df11 -a 0 -s 0x08007000:leave -D marvin.bin