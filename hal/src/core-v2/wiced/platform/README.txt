------------------------------------------------------------------------------------------------
README for Platform Directory
------------------------------------------------------------------------------------------------

This directory holds platform specific code that does not form part of the WICED Wi-Fi driver.
The directory contains the following material 

- CPU Architecture source e.g. ARM_CM3, ARM_CM4
  - CRT0 startup code
- Compiler specific source e.g. GCC, IAR
  - libc, stdio interface, etc
- Prototype definitions for the WICED platform interface
- MCU family & specific functionality
  - WWD bus initialisation
  - platform interface implementation
  - peripheral drivers
  - interrupt handlers
  - linker scripts