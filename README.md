# marvin

Spark firmware for the STM32 with CC3000

Development Environment Setup:

1. Install GCC (GNU Compiler Collection) for ARM Cortex processors:
	 https://launchpad.net/gcc-arm-embedded

2. Add path to environment variable.

3. Make sure you are able to run "make" from the terminal window.
   In Windows it needs to be explicitly installed.

4. Install Eclipse CDT

5. In Eclipse, go to (Help > Install New Software).
   click the add button, and insert the following text:

   * Name: GNU ARM Eclipse Plug-in
   * Location: http://gnuarmeclipse.sourceforge.net/updates

   Click OK and a component named "CDT GNU Cross Development Tools" will appear,
   check it, then click the Next button and follow the installation instructions.

6. Follow the screenshots in the folder "eclipse_marvin_setup"(Steps: 1 to 30)
   PS: The screenshots are not available within the GIT project(probably need to be put in Dropbox).
   
7. Important Project Settings when building for various platforms/boards:

In marvin\inc\platforn_config.h, uncomment the line corresponding to the platform/board used,
For eg.: When building for TV-1 board, except "#define USE_SPARK_TV1" all other #defines should be commented.

In Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Compiler -> Preprocessor -> Defined symbols(-D), Add the following :
* "STM32F10X_MD_VL" when building for STM32F100C8 based TV-1 board or STM32VLDiscovery board.
* "STM32F10X_MD" when building for STM32F103C8 based Core board or STM32-H103 board.

In Project Properties -> C/C++ Build -> Settings -> Tool Settings -> ARM Sourcery Windows GCC C Linker -> General -> Script file (-T), Browse & select linker file :
* "linker_stm32f10x_md_vl.ld" when building for STM32F100C8 based TV-1 board or STM32VLDiscovery board.
* "linker_stm32f10x_md.ld" when building for STM32F103C8 based Core board or STM32-H103 board.



