How does the bootloader work?
====
This bootloader is designed to simply transfer firmware binaries to and from various locations that include STM32's internal flash memory, the external SPI flash memory  and the USB port.

The bootloader is the first thing that is executed upon a system reset. It resides at the start address of the microcontroller which is at 0x08000000.

---

###Program Flow

* Initialize the system
* Load system flags
* Check system flags and select a mode (one of the following):
      * OTA Successful
      * OTA Fail
      * Factory reset mode
      * Enter DFU Mode
* Check for MODE button status and set the appropriate flags
* Take decisions based on the status of the flags set/read previously
      * If OTA was successful -> Load FW from OTA location
      * If Factory reset mode was selected -> Load FW from FAC address
      * If OTA failed -> Load FW from BKP address
* Check if a FW is available. If yes, jump to that address
* If no, enter the DFU mode
* While in the DFU mode, if the MODE button is pressed for more than 1 second and there is no FW transfer in progress, then reset the Core

---

**Device Firmware Upgrade Mode (aka DFU Mode):**
The is the default or the fall back mode for the bootloader. In this mode, the bootloader waits for the user to initiate a firmware transfer from the host computer to the Core via the USB port.
The DFU mode is triggered either when the user presses the MODE button for 3 seconds upon resetting the Core or when both Factory Reset and OTA update fail.

**Factory Reset Mode:**
This mode is triggered when the user presses the MODE button for more than 10 seconds upon resetting the Core. After entering this mode, the bootloader takes a backup of the current firmware to EXTERNAL_FLASH_BKP_ADDRESS location and then it restores the factory firmware from the location at EXTERNAL_FLASH_FAC_ADDRESS.

**Over The Air Mode (aka OTA Mode):**
This mode is very similar to the factory reset mode. Upon detecting that the OTA_UPDATE_MODE flag is true, the bootloader takes a back up of the current firmware to EXTERNAL_FLASH_BKP_ADDRESS and the copies the firmware from EXTERNAL_FLASH_OTA_ADDRESS to the internal flash memory of the STM32.

---

###System Constants

USB_DFU_ADDRESS = 0x08000000
CORE_FW_ADDRESS = 0x08005000
Internal Flash memory address where various firmwares are located

SYSTEM_FLAGS_ADDRESS = 0x08004C00
Internal Flash memory address where the System Flags will be saved and loaded from

INTERNAL_FLASH_END_ADDRESS = 0x08020000
For 128KB Internal Flash: Internal Flash end memory address

INTERNAL_FLASH_PAGE_SIZE = 0x400
Internal Flash page size

EXTERNAL_FLASH_BLOCK_SIZE = 0x20000
External Flash block size allocated for firmware storage

EXTERNAL_FLASH_FAC_ADDRESS = EXTERNAL_FLASH_BLOCK_SIZE
External Flash memory address where Factory programmed core firmware is located

EXTERNAL_FLASH_BKP_ADDRESS = EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_FAC_ADDRESS
External Flash memory address where core firmware will be saved for backup/restore

EXTERNAL_FLASH_OTA_ADDRESS = EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_BKP_ADDRESS
External Flash memory address where OTA upgraded core firmware will be saved

EXTERNAL_FLASH_SERVER_PUBLIC_KEY_ADDRESS  = 0x01000
External Flash memory address where server public RSA key resides

EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH = 294
Length in bytes of DER-encoded 2048-bit RSA public key

EXTERNAL_FLASH_CORE_PRIVATE_KEY_ADDRESS = 0x02000
External Flash memory address where core private RSA key resides

EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH = 612
Length in bytes of DER-encoded 1024-bit RSA private key

---


###Backup Registers

**BKP_DR10:**
*0x5000*
0x5000 is written to the backup register after transferring the FW from the external flash to the STM32's internal memory

*0x0005*
0x0005 is written to the backup register at the end of firmware update process. If the register reads 0x0005, it signifies that the firmware update was successfully completed.

*0x5555*
0x5555 is written to the backup register at the beginning of firmware update process. If the register still reads 0x5555 signifies that the firmware update was never completed.

---

###Memory Maps

**External Flash Memory Map**
<table border = '1'>
   <tr>
      <th>Memory Address</th>
      <th>Content</th>
      <th>Size</th>
   </tr>
   <tr>
      <td>0x00000</td>
      <td>Reserved</td>
      <td>4KB</td>
   </tr>
   <tr>
      <td>0x01000</td>
      <td>Public Key</td>
      <td>294 Bytes - 4KB max</td>
   </tr>
   <tr>
      <td>0x02000</td>
      <td>Private Key</td>
      <td>612 Bytes</td>
   </tr>
   <tr>
      <td>0x20000</td>
      <td>Factory Reset Firmware Location</td>
      <td>128 KB max</td>
   </tr>
   <tr>
      <td>0x40000</td>
      <td>BackUp Firmware Location</td>
      <td>128 KB max</td>
   </tr>
   <tr>
      <td>0x60000</td>
      <td>OTA Firmware Location</td>
      <td>128 KB max</td>
   </tr>
   <tr>
      <td>0x80000</td>
      <td>End of OTA Firmware</td>
   </tr>
   <tr>
      <td> </td>
      <td> NOT USED </td>
   </tr>
   <tr>
      <td>0x200000</td>
      <td>End of Flash Memory</td>
   </tr>
</table>

**Internal Flash Memory Map**
<table border = '1'>
   <tr>
      <th>Memory Address</th>
      <th>Content</th>
      <th>Size</th>
   </tr>
   <tr>
      <td>0x08000000</td>
      <td>Bootloader</td>
      <td>19 KB max</td>
   </tr>
   <tr>
      <td>0x08004C00</td>
      <td>System Flags</td>
      <td>1 KB max</td>
   </tr>
   <tr>
      <td>0x08005000</td>
      <td>Core Firmware Location</td>
      <td>108 KB max</td>
   </tr>
</table>

