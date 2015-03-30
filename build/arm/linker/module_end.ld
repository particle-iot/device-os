    .module_info_suffix :
    {
        . = ALIGN(., 4) ;
        link_module_info_suffix_start = .;
        KEEP(*module_info.o(.modinfo.module_info_suffix))
        link_module_info_suffix_end = .;
    }>FLASH 

    .module_end : 
    {
        link_module_end = .;
    }> FLASH 

    .module_info_crc :
    {
        link_module_info_crc_start = .;
        KEEP(*module_info.o(.modinfo.module_info_crc))
        link_module_info_crc_end = .;
    }>FLASH
