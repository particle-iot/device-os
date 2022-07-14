__rtos_capture_start = .;
*tasks.o(.bss COMMON .bss*)
*timers*.o(.bss COMMON .bss*)
__rtos_capture_end = .;
