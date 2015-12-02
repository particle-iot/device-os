# Module Info

The `module_info.h` file describes the structure of the block of metadata (module info) that is
inserted into modules supporting self-describing memory location and CRC checks.

The structure is initialized by the code in `module_info.c` and linked at a specific
location by the linker script. The linker script also provides the addresses of the
start of the module and the end of the module.

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

The binaries consumed in the Particle ecosystem should include "module info" descriptors that appear at fixed locations in the binary. There are 2 module info structures:

- start info: appears at the start of the binary, this defines the platform, version, module type, module dependencies
- end info: appears at the end of the binary. Defines product ID and version, as set in user code and provides storage for the SHA256 and CRC32 hashes that are added after the binary is built.


# Compiling Module Info

The module info structures are defined in `dynalib/inc/module_info.h`.  The structures are instantiated in `dynalib/inc/module_info.inc` which defines the default values for all platforms.

The `main` project compiles the module info structure instances into a `module_info.o` object file - `main/src/module_info.c` simply includes `module_info.inc` from dynalib so the instance declarations become static data.

## Linking Module Info

Your project's linker script describes how the various object files are placed into the final firmware image. The module infos need to be placed at specific locations, and so we must use linker scripting to instruct the linker to place these at the desired locations.

We provide some linker scripts to make this process simple (and avoid copy/pasting code.) All the linker scripts are stored in `build/arm/linker`. If not already done, you should add this directory to the linker search path with

```
LDFLAGS += -L$(COMMON_BUILD)/arm/linker
```

Add this alongside the other linker flags defines in your HAL `include.mk` file.


All the linker script changes add data to the program memory region - this is where the main executable code is stored. To reuse our linker scripts that inject the module info, this section should be named `APP_FLASH`. For example,

```
.somesection :
{
  // .. declarations
} > APP_FLASH

.text :
{
  // .. more declarations
} > APP_FLASH
```


### Module Start Symbol

A symbol for the module start is used to compute various offsets.

Add this include before any other APP_FLASH section.
```
INCLUDE 'module_start.ld'
```

In the example, this would come before `.somesection`.

### Module Info Start

After any mandatory sections in program memory, such as ISR vector tables, add

```
INCLUDE 'module_info.ld'
```

In the example, this would come after `.somesection` if that section must remain at the start of the binary image. If `.somesection` can appear anywhere, the include would be placed before it so that the module info is placed at the very beginning of the module.


### Module Info End

After all other `APP_FLASH` sections, add

```
INCLUDE 'module_end.ld'
```

In the example, this would be placed after the `.text` section.



## Troubleshooting

Without the module info end structures in place, building `main` will fail since the module info end data isn't present. This includes default SHA256 and CRC32 values that are built into the binary, and the buidl will fail when those do not match the values expected.  This is a sanity check in the build that it's overwriting the correct data when writing the SHA256 and CRC32 hashes to offsets from the end of the file.





