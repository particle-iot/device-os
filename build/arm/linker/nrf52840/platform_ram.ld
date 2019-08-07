/*
Modular firmware:

+--------------------------+ 0x20040000
| User backup RAM          |
+--------------------------+ 0x2003f400
| System flags             |
+--------------------------+ 0x2003f3c0
| Backup registers         |
+--------------------------+ 0x2003f380
| System backup RAM        |
+--------------------------+ 0x2003f000
| Main stack               |
+--------------------------+ 0x2003e800
| User part static RAM     |
+~~~~~~~~~~~~~~~~~~~~~~~~~~+
|                          |
| Heap                     |
|                          |
+~~~~~~~~~~~~~~~~~~~~~~~~~~+
| System part 1 static RAM |
+--------------------------+ 0x20004000
| SoftDevice RAM           |
+--------------------------+ 0x20000000

TODO: Bootloader, monolithic firmware

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
_ram_size = 256K;
_ram_start = 0x20000000;
_ram_end = _ram_start + _ram_size;

/* User backup memory */
_user_backup_ram_size = 3K;
_user_backup_ram_end = _ram_end;
_user_backup_ram_start = _user_backup_ram_end - _user_backup_ram_size;

/* System flags */
_system_flags_ram_size = 64;
_system_flags_ram_end = _user_backup_ram_start;
_system_flags_ram_start = _system_flags_ram_end - _system_flags_ram_size;

/* Backup registers */
_backup_registers_ram_size = 64;
_backup_registers_ram_end = _system_flags_ram_start;
_backup_registers_ram_start = _backup_registers_ram_end - _backup_registers_ram_size;

/* System backup memory */
_system_backup_ram_size = 4K - _user_backup_ram_size - _system_flags_ram_size - _backup_registers_ram_size;
_system_backup_ram_end = _backup_registers_ram_start;
_system_backup_ram_start = _system_backup_ram_end - _system_backup_ram_size;

/* Main stack */
_stack_size = __STACKSIZE__; /* Defined in modules/shared/nRF52840/part1_build.mk */
_stack_end = _system_backup_ram_start;
_stack_start = _stack_end - _stack_size; /* The lowest address of the stack */

/* User part */
_user_part_static_ram_size = DEFINED(_user_part_static_ram_size) ? _user_part_static_ram_size : 89K; /* Defined in platform_user_ram.ld */
_user_part_static_ram_end = _stack_start;
_user_part_static_ram_start = _user_part_static_ram_end - _user_part_static_ram_size;

/* SoftDevice */
_softdevice_ram_size = 23K;
_softdevice_ram_start = _ram_start;
_softdevice_ram_end = _softdevice_ram_start + _softdevice_ram_size;

/* System part 1 */
_system_part1_static_ram_size = 100K;
_system_part1_static_ram_start = _softdevice_ram_end;
_system_part1_static_ram_end = _system_part1_static_ram_start + _system_part1_static_ram_size;

/* Heap */
_heap_min_size = 32K;

/* Basic sanity checks */
ASSERT(
    _softdevice_ram_end - _softdevice_ram_start == _softdevice_ram_size && _softdevice_ram_size > 0 &&
    _system_part1_static_ram_end - _system_part1_static_ram_start == _system_part1_static_ram_size && _system_part1_static_ram_size > 0 &&
    _user_part_static_ram_end - _user_part_static_ram_start == _user_part_static_ram_size && _user_part_static_ram_size > 0 &&
    _stack_end - _stack_start == _stack_size && _stack_size > 0 &&
    _system_backup_ram_end - _system_backup_ram_start == _system_backup_ram_size && _system_backup_ram_size > 0 &&
    _backup_registers_ram_end - _backup_registers_ram_start == _backup_registers_ram_size && _backup_registers_ram_size > 0 &&
    _system_flags_ram_end - _system_flags_ram_start == _system_flags_ram_size && _system_flags_ram_size > 0 &&
    _user_backup_ram_end - _user_backup_ram_start == _user_backup_ram_size && _user_backup_ram_size > 0 &&
    _ram_end - _ram_start == _ram_size && _ram_size > 0,
    "Invalid memory map");
