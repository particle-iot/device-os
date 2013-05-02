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

8. Update the "marvin" OR other user application project for loading via DFU as follows:
In hw_config.c, update NVIC_Configuration() function as shown below
replace:
NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0000);
with:
NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x7000);

In linker_stm32f10x_md.ld, update MEMORY section as shown below
replace:
FLASH (rx)    : ORIGIN = 0x08000000, LENGTH = 128K
with:
FLASH (rx)    : ORIGIN = 0x08007000, LENGTH = 128K-0x7000

9. Build the marvin "project".

10. Create a marvin.dfu or host plaform's DFU tool specific file from the generated marvin.hex or marvin.bin file: TO BE CONTINUED