/*
Photon RAM (modular firmware):

+-------------------+ 0x20020000
| Main stack        |
+-------------------+ 0x2001f000
| system-part2 SRAM |
+-------------------+ 0x20014800
| system-part1 SRAM |
+-------------------+ 0x20014400
|                   |
|                   |
| Heap              |
|                   |
|                   |
+~~~~~~~~~~~~~~~~~~~+
| user-part SRAM    |
+-------------------+ 0x20000000

Note: Keep the diagram up to date with the actual memory map defined in this file.
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
_part2_static_ram_size = 42K;
_part2_static_ram_end = _stack_start;
_part2_static_ram_start = _part2_static_ram_end - _part2_static_ram_size;

/* System part 1 */
_part1_static_ram_size = 1K;
_part1_static_ram_end = _part2_static_ram_start;
_part1_static_ram_start = _part1_static_ram_end - _part1_static_ram_size;

/* User part */
_user_static_ram_start = _ram_start;
_user_static_ram_end = _part1_static_ram_start;
_user_static_ram_size = _user_static_ram_end - _user_static_ram_start;

/* Heap memory */
_heap_start = _ram_start;
_heap_end = _part1_static_ram_start;
_heap_size = _heap_end - _heap_start;
_heap_min_size = 16K;

/* Generic definitions independent of the number of system modules */
_system_static_ram_start = _part1_static_ram_start;
_system_static_ram_end = _part2_static_ram_end;
_system_static_ram_size = _system_static_ram_end - _system_static_ram_start;
