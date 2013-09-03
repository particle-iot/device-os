/*
* STM32 Linker Script
*/

_estack = 0x20005000;    /* global needed for startup_stm3210x_md_vl.S */
_minimum_stack_size = 0x100; /* 256 bytes miniumum stack size */

ENTRY(Reset_Handler)

MEMORY
{
  rom (rx)      : ORIGIN = 0x0800C000, LENGTH = 128K-0xC000
  ram (xrw)     : ORIGIN = 0x20000000, LENGTH = 20K
}

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
  } >rom

   /* .ARM.exidx is sorted, so has to go in its own output section.*/
   .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } >rom

    _sidata = .;         /* global needed for startup_stm3210x_md_vl.S */  

  /*now start ram area with initialized data section */
  .data :
  {   
    _sdata = .;        /* global needed for startup_stm3210x_md_vl.S */
    *(.data)          
    *(.data*)         
    . = ALIGN(4);
    _edata = .;        /* global needed for startup_stm3210x_md_vl.S */
  } >ram AT>rom

  /* uninitialized data section bss*/
  .bss :
  {
      . = ALIGN(4);
    _sbss = .;         /* global needed for startup_stm3210x_md_vl.S */
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;        /* global needed for startup_stm3210x_md_vl.S */
  } >ram

 
  /* Generates an error if minimum stack size is not left in ram */
  ._check_stack_space :
  {
    . = ALIGN(4);
    . = . + _minimum_stack_size;
    . = ALIGN(4);
  } >ram

  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }

  /* Remove debug information from the standard libraries */
  /DISCARD/ : {
      *(.note.GNU-stack)
    libc.a ( * )
    libgcc.a ( * )
    libm.a ( * )
  }
}
