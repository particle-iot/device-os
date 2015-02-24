    .module_info :
    {
        . = ALIGN(4);
        link_module_info_start = .;
        KEEP(*module_info.o(.rodata.module_info))
        link_module_info_end = .;
    }>FLASH
