/*
Platform FLASH memory map for Argon, Boron, Xenon, ASoM, BSoM, XSoM.

Modular firmware:                          Monolithic firmware:

+--------------------------------+ 0x00100000 +----------------------------------+
|                                |            |                                  |
| Bootloader                     |            | Bootloader                       |
|                                |            |                                  |
+--------------------------------+ 0x000F4218 +----------------------------------+
| Bootloader module info         |            | Bootloader module info           |
+--------------------------------+ 0x000F4200 +----------------------------------+
| Bootloader vector table        |            | Bootloader vector table          |
+--------------------------------+ 0x000F4000 +----------------------------------+
|             | User part        |            |                                  |
|             | compat           |            |                                  |
|             |                  |            |                                  |
|             |~~~~~~~~~~~~~~~~~~+            |                                  |
|             | dynalib compat   |            |                                  |
|             |------------------+ 0x000D4018 |                                  |
|             | mod info compat  |            |                                  |
|             `------------------+ 0x000D4000 |                                  |
|                                |            |                                  |
| (Extended) User part           |            |                                  |
|                                |            |                                  |
|                                |            | Monolithic firmware              |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part dynalib table        |            |                                  |
+--------------------------------+ 0x000B4018 |                                  |
| User part module info          |            |                                  |
+--------------------------------+ 0x000B4000 |                                  |
|                                |            |                                  |
| System part 1                  |            |                                  |
|                                |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 dynalib table    |            |                                  |
+--------------------------------+ 0x00030218 +----------------------------------+
| System part 1 module info      |            | Monolithic firmware module info  |
+--------------------------------+ 0x00030200 +----------------------------------+
| System part 1 vector table     |            | Monolithic firmware vector table |
+--------------------------------+ 0x00030000 +----------------------------------+
| Reserved for SoftDevice        |            | Reserved for SoftDevice          |
+--------------------------------+ 0x00026000 +----------------------------------+
| SoftDevice                     |            | SoftDevice                       |
+--------------------------------+ 0x00000000 +----------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Platform interrupt vector table size. Please reference to the interrupt vector table in platform startup file. */
platform_vector_table_flash_size = 512;

/* Module info size */
platform_module_info_flash_size = 24;

/* Total amount of flash available on the MCU */
platform_flash_size  = 1M;
platform_flash_start = 0x00000000;
platform_flash_end   = platform_flash_start + platform_flash_size;


/* Bootloader */
platform_bootloader_flash_size  = 48K;
platform_bootloader_flash_end   = platform_flash_end;
platform_bootloader_flash_start = platform_bootloader_flash_end - platform_bootloader_flash_size;

/* Bootloader vector table */
platform_bootloader_vector_table_flash_size  = platform_vector_table_flash_size;
platform_bootloader_vector_table_flash_start = platform_bootloader_flash_start;
platform_bootloader_vector_table_flash_end   = platform_bootloader_vector_table_flash_start + platform_bootloader_vector_table_flash_size;

/* Bootloader module info */
platform_bootloader_module_info_flash_size  = platform_module_info_flash_size;
platform_bootloader_module_info_flash_start = platform_bootloader_vector_table_flash_end;
platform_bootloader_module_info_flash_end   = platform_bootloader_module_info_flash_start + platform_bootloader_module_info_flash_size;


/* User part */
platform_user_part_flash_size  = 256K;
platform_user_part_flash_end   = platform_bootloader_flash_start;
platform_user_part_flash_start = platform_user_part_flash_end - platform_user_part_flash_size;

/* User part module info */
platform_user_part_module_info_flash_size  = platform_module_info_flash_size;
platform_user_part_module_info_flash_start = platform_user_part_flash_start;
platform_user_part_module_info_flash_end   = platform_user_part_module_info_flash_start + platform_user_part_module_info_flash_size;

/* User part dynalib table */
platform_user_part_dynalib_table_flash_start = platform_user_part_module_info_flash_end;

/* User part compat */
platform_user_part_compat_flash_size  = 128K;
platform_user_part_compat_flash_end   = platform_bootloader_flash_start;
platform_user_part_compat_flash_start = platform_user_part_compat_flash_end - platform_user_part_compat_flash_size;

/* User part compat module info */
platform_user_part_compat_module_info_flash_size  = platform_module_info_flash_size;
platform_user_part_compat_module_info_flash_start = platform_user_part_compat_flash_start;
platform_user_part_compat_module_info_flash_end   = platform_user_part_compat_module_info_flash_start + platform_user_part_compat_module_info_flash_size;

/* User part compat dynalib table */
platform_user_part_compat_dynalib_table_flash_start = platform_user_part_compat_module_info_flash_end;

/* System part 1 */
platform_system_part1_flash_size  = 528K;
platform_system_part1_flash_end   = platform_user_part_flash_start;
platform_system_part1_flash_start = platform_system_part1_flash_end - platform_system_part1_flash_size;

/* System part 1 vector table */
platform_system_part1_vector_table_flash_size  = platform_vector_table_flash_size;
platform_system_part1_vector_table_flash_start = platform_system_part1_flash_start;
platform_system_part1_vector_table_flash_end   = platform_system_part1_vector_table_flash_start + platform_system_part1_vector_table_flash_size;

/* System part 1 module info */
platform_system_part1_module_info_flash_size  = platform_module_info_flash_size;
platform_system_part1_module_info_flash_start = platform_system_part1_vector_table_flash_end;
platform_system_part1_module_info_flash_end   = platform_system_part1_module_info_flash_start + platform_system_part1_module_info_flash_size;

/* System part 1 dynalib table */
platform_system_part1_dynalib_table_flash_start = platform_system_part1_module_info_flash_end;


/* Monolithic firmware*/
platform_monolithic_firmware_flash_size  = 784K;
platform_monolithic_firmware_flash_end   = platform_bootloader_flash_start;
platform_monolithic_firmware_flash_start = platform_monolithic_firmware_flash_end - platform_monolithic_firmware_flash_size;

/* Monolithic firmware vector table */
platform_monolithic_firmware_vector_table_flash_size  = platform_vector_table_flash_size;
platform_monolithic_firmware_vector_table_flash_start = platform_monolithic_firmware_flash_start;
platform_monolithic_firmware_vector_table_flash_end   = platform_monolithic_firmware_vector_table_flash_start + platform_monolithic_firmware_vector_table_flash_size;

/* Monolithic firmware module info */
platform_monolithic_firmware_module_info_flash_size  = platform_module_info_flash_size;
platform_monolithic_firmware_module_info_flash_start = platform_monolithic_firmware_vector_table_flash_end;
platform_monolithic_firmware_module_info_flash_end   = platform_monolithic_firmware_module_info_flash_start + platform_monolithic_firmware_module_info_flash_size;


/* Reserved for Softdevice extention */
platform_softdevice_reserved_flash_size  = 40K;
platform_softdevice_reserved_flash_end   = platform_system_part1_flash_start;
platform_softdevice_reserved_flash_start = platform_softdevice_reserved_flash_end - platform_softdevice_reserved_flash_size;

/* Softdevice */
platform_softdevice_flash_size  = 152K;
platform_softdevice_flash_start = platform_flash_start;
platform_softdevice_flash_end   = platform_softdevice_flash_start + platform_softdevice_flash_size;


/* Basic sanity checks */
ASSERT (
    (platform_flash_end - platform_flash_start == platform_flash_size && platform_flash_size > 0) &&
    (platform_softdevice_flash_size + platform_softdevice_reserved_flash_size + platform_system_part1_flash_size + platform_user_part_flash_size + platform_bootloader_flash_size == platform_flash_size) &&
    (platform_softdevice_flash_size + platform_softdevice_reserved_flash_size + platform_monolithic_firmware_flash_size + platform_bootloader_flash_size == platform_flash_size),
    "Invalid overall flash memory map!"
);
ASSERT (
    (platform_bootloader_flash_start == 0xF4000) &&
    (platform_bootloader_flash_end - platform_bootloader_flash_start == platform_bootloader_flash_size && platform_bootloader_flash_size > 0) &&
    (platform_bootloader_vector_table_flash_end - platform_bootloader_vector_table_flash_start == platform_bootloader_vector_table_flash_size && platform_bootloader_vector_table_flash_size > 0) &&
    (platform_bootloader_module_info_flash_end - platform_bootloader_module_info_flash_start == platform_bootloader_module_info_flash_size && platform_bootloader_module_info_flash_size > 0),
    "Invalid bootloader flash memory map!"
);
ASSERT (
    (platform_user_part_flash_start == 0xB4000) &&
    (platform_user_part_flash_end - platform_user_part_flash_start == platform_user_part_flash_size && platform_user_part_flash_size > 0) &&
    (platform_user_part_module_info_flash_end - platform_user_part_module_info_flash_start == platform_user_part_module_info_flash_size && platform_user_part_module_info_flash_size > 0),
    "Invalid user part flash memory map!"
);
ASSERT (
    (platform_user_part_compat_flash_start == 0xD4000) &&
    (platform_user_part_compat_flash_end - platform_user_part_compat_flash_start == platform_user_part_compat_flash_size && platform_user_part_compat_flash_size > 0) &&
    (platform_user_part_compat_module_info_flash_end - platform_user_part_compat_module_info_flash_start == platform_user_part_compat_module_info_flash_size && platform_user_part_compat_module_info_flash_size > 0),
    "Invalid user part compat flash memory map!"
);
ASSERT (
    (platform_system_part1_flash_start == 0x30000) &&
    (platform_system_part1_flash_end - platform_system_part1_flash_start == platform_system_part1_flash_size && platform_system_part1_flash_size > 0) &&
    (platform_system_part1_vector_table_flash_end - platform_system_part1_vector_table_flash_start == platform_system_part1_vector_table_flash_size && platform_system_part1_vector_table_flash_size > 0) &&
    (platform_system_part1_module_info_flash_end - platform_system_part1_module_info_flash_start == platform_system_part1_module_info_flash_size && platform_system_part1_module_info_flash_size > 0),
    "Invalid system part 1 flash memory map!"
);
ASSERT (
    (platform_monolithic_firmware_flash_start == 0x30000) &&
    (platform_monolithic_firmware_flash_end - platform_monolithic_firmware_flash_start == platform_monolithic_firmware_flash_size && platform_monolithic_firmware_flash_size > 0) &&
    (platform_monolithic_firmware_vector_table_flash_end - platform_monolithic_firmware_vector_table_flash_start == platform_monolithic_firmware_vector_table_flash_size && platform_monolithic_firmware_vector_table_flash_size > 0) &&
    (platform_monolithic_firmware_module_info_flash_end - platform_monolithic_firmware_module_info_flash_start == platform_monolithic_firmware_module_info_flash_size && platform_monolithic_firmware_module_info_flash_size > 0),
    "Invalid monolithic firmware flash memory map!"
);
/* NOTE: there is a reserved MBR parameter page at the end of the allocated SoftDevice area (192K - 4K) */
/* This page address is dependent on platform_softdevice_reserved_flash_end. Do not modify! */
ASSERT (
    (platform_softdevice_reserved_flash_end == 0x30000) &&
    (platform_softdevice_reserved_flash_end - platform_softdevice_reserved_flash_start == platform_softdevice_reserved_flash_size && platform_softdevice_reserved_flash_size > 0) &&
    (platform_softdevice_flash_end - platform_softdevice_flash_start == platform_softdevice_flash_size && platform_softdevice_flash_size > 0),
    "Invalid softdevice flash memory map!"
);
