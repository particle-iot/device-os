# Module Info

The `module_info.h` file describes the structure of the module into that is 
inserted into modules supporting self-describing placement and CRC checks.

The structure is initialized via `module_info.c` and link at a specific 
location by the linker script. The linker script provides the locations for the very
start of the module and the very end of the module.

## Locating the module info struct

The module info struct is located like this:

- examine the first 4 address at the start of the module. 

- If this is a module with an interrupt vector table (VTOR) the first address 
will contain the top of the stack in ram (0x20xxxxxx). In this case the info table
will be placed immediately after the VTOR table. For STM32F205, the VTOR table 
comprises 97 interrupt vectors, giving a total size of 388 (0x184) bytes.

- The other case is a module without the VTOR, where the module info appears at
the very start of the module. In this case the first 4 byte address is the first item
in the module info structure, which is the start location of the module in Flash
(0x08xxxxxx) where the module resides/should reside.

Once the location of the module_info structure has been determinted, the address
can be cast and assigned to a `const module_info_t*`

```
    #include "module_info.h"

    void* address = (void*)0x8020000;
    if ((*address & 0x20000000)==0x20000000)
        address = (void*)0x8020184;
    const module_info_t* module_info = (const module_info*)address;
```

