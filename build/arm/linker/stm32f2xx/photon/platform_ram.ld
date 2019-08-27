/*
Modular firmware:

+--------------------------+ 0x20020000
| Main stack               |
+--------------------------+ 0x2001f000
| System part 2 static RAM |
+--------------------------+ 0x20014800
| System part 1 static RAM |
+--------------------------+ 0x20014400
|                          |
| Heap                     |
|                          |
+~~~~~~~~~~~~~~~~~~~~~~~~~~+
| User part static RAM     |
+--------------------------+ 0x20000000

TODO: Backup memory, bootloader, monolithic firmware

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
_ram_size = 128K;
_ram_start = 0x20000000;
_ram_end = _ram_start + _ram_size;

/* Main stack */
_stack_size = 4K;
_stack_end = _ram_end;
_stack_start = _stack_end - _stack_size; /* The lowest address of the stack */

/* System part 2 */
_system_part2_static_ram_size = 42K;
_system_part2_static_ram_end = _stack_start;
_system_part2_static_ram_start = _system_part2_static_ram_end - _system_part2_static_ram_size;

/* System part 1 */
_system_part1_static_ram_size = 1K;
_system_part1_static_ram_end = _system_part2_static_ram_start;
_system_part1_static_ram_start = _system_part1_static_ram_end - _system_part1_static_ram_size;

/* User part */
_user_part_static_ram_start = _ram_start;
_user_part_static_ram_end = _system_part1_static_ram_start;
_user_part_static_ram_size = _user_part_static_ram_end - _user_part_static_ram_start;

/* Heap memory */
_heap_size = _user_part_static_ram_size; /* Occupies the same region as the user RAM initially */
_heap_start = _user_part_static_ram_start;
_heap_end = _user_part_static_ram_end;
_heap_min_size = 16K;

/* Generic definitions independent of the number of system modules */
_system_static_ram_start = _system_part1_static_ram_start;
_system_static_ram_end = _system_part2_static_ram_end;
_system_static_ram_size = _system_static_ram_end - _system_static_ram_start;

/* Basic sanity checks */
ASSERT(
    _user_part_static_ram_end - _user_part_static_ram_start == _user_part_static_ram_size && _user_part_static_ram_size > 0 &&
    _system_part1_static_ram_end - _system_part1_static_ram_start == _system_part1_static_ram_size && _system_part1_static_ram_size > 0 &&
    _system_part2_static_ram_end - _system_part2_static_ram_start == _system_part2_static_ram_size && _system_part2_static_ram_size > 0 &&
    _system_static_ram_end - _system_static_ram_start == _system_static_ram_size && _system_static_ram_size > 0 &&
    _heap_end - _heap_start == _heap_size && _heap_size > 0 && _heap_min_size > 0 &&
    _stack_end - _stack_start == _stack_size && _stack_size > 0 &&
    _ram_end - _ram_start == _ram_size && _ram_size > 0,
    "Invalid memory map");
