# Dynalib

Device OS is built as multiple binaries: one or more system firmware binaries and one user firmware binary.

System firmware has APIs that the user firmware calls to interface with the peripherals, cloud connection, etc. The machanism for exposing those APIs across binaries compiled separately is called **Dynalib** (dynamic library).

Each module of system firmware (communication, crypto, hal, rt, services, system) lists the functions that it wants to export through a set of macros.

When system firmware is built, Dynalib is compiled with `DYNALIB_EXPORT` and it creates a table of function pointers that will be included in the firmware binary at a known memory address.

When user firmware is built, Dynalib is compiled with `DYNALIB_IMPORT` and it creates a set of function stubs that will load the function pointer from system firwmare and call it.

## Dynalib example

This is an example Dynalib table:

```
DYNALIB_BEGIN(hal_spi)

DYNALIB_FN(0, hal_spi, HAL_SPI_Begin, void(HAL_SPI_Interface, uint16_t))
// exports 1 to 16 here
DYNALIB_FN(17, hal_spi, HAL_SPI_Release, int32_t(HAL_SPI_Interface, void*))
DYNALIB_END(hal_spi)
```

The first `DYNALIB_FN` line makes the function `HAL_SPI_Begin` available to be called by the user firmware. The function takes `HAL_SPI_Interface` and `uint16_t` arguments and returns `void`.

The export table in system firmware will look like this:
```
const void* const dynalib_hal_spi[] = {
    (const void*)&HAL_SPI_Begin,
    // exports 1 to 16 here
    (const void*)&HAL_SPI_Release,
};
```

The import stubs in user firmware will look like this:
```
extern const void* const dynalib_hal_spi[];
void HAL_SPI_Begin() __attribute__((naked));
void HAL_SPI_Begin() {
    asm volatile (
        // assembler to fetch the address of HAL_SPI_Begin from dynalib_hal_spi and jump to it
    );
}
```

## Rules for adding to dynalib

- Always add new functions to the end of the list
- Never delete a function from the list
- Never replace a function with another one
- Don't change the arguments list for a function. The only exception is **changing pointer types is allowed.**
- In order to make dynalib functions extendable in the future, it's a good idea to add an unused `void*` argument like in the `HAL_SPI_Release` example above. Initially callers pass `nullptr` as the value of that argument. Later if additional arguments are needed, change the pointer type to a struct and put additional arguments in there.
- Don't change the return value of a function. The only exception is **changing a `void` return to a non-`void` return.**

## Application Binary Interface (ABI) Compatibility

Device OS needs to be both backward and forward compatible to user firmware across different versions.

| |Old user firmware | New user firmware
|-|-|-|
**Old system firmware** | Runs normally. | User firmware may use Dynalib functions not present in old system firmware. Enter safe mode so user firmware does not run.
**New system firmware** | User firmware can only use old Dynalib functions. Runs normally. | Runs normally.

Backwards compatibility means that when an old user firmware runs against a new system firmware, the user application will be able to find the same Dynalib functions at the same offsets. Dynalib functions added to the end of the table don't impact user application.

Forward compatibility means not crashing when a new user firmware runs against an old system firmware. A new application may try to call a Dynalib function that doesn't exist in the old system firmware. This would result in loading an invalid function pointer and calling it would crash the device. To prevent this, Device OS checks if the user firmware was compiled against a newer Device OS version than the system part present on the device. If that is the case, the device goes into safe mode and will not run the user application.