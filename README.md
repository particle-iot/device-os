# dfu

Spark USB-DFU firmware for the STM32

Development Environment Setup:

1. Copy .cproject and .project files from "marvin" into "dfu" folder.

2. Search and replace "marvin" with "dfu" in the above 2 files.

3. In Eclipse, select File -> Import -> Existing Projects into Workspace -> Select root directry: -> Browse and select "dfu" folder -> Finish.

4. Build the project and load the dfu.hex or dfu.bin file via JTAG.

5. The device should appear as "STM Device in DFU Mode" on the Host platform.

6. Build the marvin project for loading via USB:- To be Continued.


