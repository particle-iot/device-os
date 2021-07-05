
# Linker Scripts in Device OS

Document herein introduces how linker scripts are organized and the naming convention of symbols in linker script.

## File Structure


- `device-os/build/arm/linker`: Shared **auxiliary** linker scripts for linking bootloader, monolithic and modular firmware for all platforms.

- `device-os/build/arm/linker/(core)`: Shared **auxiliary** linker scripts for linking bootloader, monolithic and modular firmware for core-based platforms.

- `device-os/build/arm/linker/(core)/(platform)`: **Auxiliary** linker scripts for linking bootloader, monolithic and modular firmware for specific platform.

- `device-os/bootloader/src/(core)`:: Linker script **entrance** for linking the bootloader for core-based platforms.

- `device-os/hal/src/(core)`: Linker scripts **entrance** for linking the monolithic firmware for core-based platforms.

- `device-os/hal/src/(platform)`: Linker scripts **entrance** for linking the monolithic firmware for specific platform.

- `device-os/modules/shared/(core)`: Shared **auxiliary** linker scripts for linking the modular firmware for core-based platforms.

- `device-os/modules/(platform)/system-part1`: Linker script **entrance** for linking the system part1 firmware for specific platform, and **glue** linker scripts for linking other parts of modular firmware.

- `device-os/modules/(platform)/system-part2`: Linker script **entrance** for linking the system part2 firmware for specific platform, and **glue** linker scripts for linking other parts of modular firmware.

- `device-os/modules/(platform)/system-part3`: Linker script **entrance** for linking the system part3 firmware for specific platform, and **glue** linker scripts for linking other parts of modular firmware.

- `device-os/modules/(platform)/user-part`: Linker script **entrance** for linking the user part firmware for specific platform, and **glue** linker scripts for linking other parts of modular firmware.


## FIle Name

- Linker script entrance is always named `linker.ld`.

- Auxiliary linker scripts those defined output section's memory region are prefixed with `memory_`, e.g. `memory_backup_ram.ld`.

- Auxiliary linker scripts those defined platform's flash memory map are named `platform_flash.ld`.

- Auxiliary linker scripts those defined platform's RAM memory map are named `platform_ram.ld`.

- Other auxiliary linker scripts are prefixed with `linker_`, e.g. `linker_foobar.ld`.

- Glue linker scripts for modular building are prefixed with `module_`, e.g. `module_system_part1_export.ld`.

## Symbols

- Symbols those are calculated by linker or referenced by source code should be prefixed with `link_`, e.g. `link_global_data_start`. This kind of symbols can also be referenced by other linker scripts if necessary, such as the symbols defined in glue linker scripts.

- Symbols those are defined in `platform_flash.ld` and `platform_ram.ld` are prefixed with `platform_`, e.g. `platform_flash_start`.

- Intermediate symbols those are referenced locally where they are defined, should be prefixed with `p_`, e.g. `p_foobar`. 

-  Intermediate symbols those are referenced by other linker scripts should be prefixed with `extern_`, e.g. `extern_foobar`.
