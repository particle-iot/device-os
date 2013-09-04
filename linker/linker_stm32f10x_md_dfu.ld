/*
*****************************************************************************
**
**  File        : linker_stm32f10x_md.ld
**
**  Abstract    : Linker script for STM32F103CB Device with
**                128KByte FLASH, 20KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Environment : Eclipse-CDT and GNU Tools ARM Embedded. 
**
*****************************************************************************
*/

/* default stack sizes. 

These are used by the startup in order to allocate stacks for the different modes.
*/

__Stack_Size = 1024 ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;

__Stack_Init = _estack - __Stack_Size ;

/*"PROVIDE" allows to easily override these values from an object file or the commmand line.*/
PROVIDE ( _Stack_Init = __Stack_Init ) ;

/*
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0x100 ;

/* include the memory spaces definitions sub-script */
/*
Linker subscript for STM32F10x definitions with 128K Flash and 20K RAM */

/* Memory Spaces Definitions */

MEMORY
{
  RAM (xrw)     : ORIGIN = 0x20000000, LENGTH = 20K
  FLASH (rx)    : ORIGIN = 0x0800C000, LENGTH = 128K-0xC000
}

/* higher address of the user mode stack */
_estack = 0x20005000;

/* include the sections management sub-script for FLASH mode */

/* Sections Definitions */

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
  } >FLASH

   /* .ARM.exidx is sorted, so has to go in its own output section.*/
   .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } >FLASH

    _sidata = .;           

  /*now start RAM area with initialized data section */
  .data :
  {   
    _sdata = .;        
    *(.data)          
    *(.data*)         
    . = ALIGN(4);
    _edata = .;        
  } >RAM AT>FLASH

  /* uninitialized data section bss*/
  .bss :
  {
      . = ALIGN(4);
    _sbss = .;         
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;        
  } >RAM

  PROVIDE ( _end = _ebss );
 
  /* Generates an error if minimum stack size is not left in RAM */
  ._check_stack_space :
  {
    . = ALIGN(4);
    . = . + _Minimum_Stack_Size;
    . = ALIGN(4);
  } >RAM

  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }

  /* Remove debug information from the standard libraries */
  /DISCARD/ : {
      *(.note.GNU-stack)
    libc.a ( * )
    libgcc.a ( * )
    libm.a ( * )
    libg.a ( * )
  }
}
