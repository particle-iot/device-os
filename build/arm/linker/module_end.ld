    .module_info_suffix :
    {
        . = ALIGN(., 4) ;
        link_module_info_suffix_start = .;
        KEEP(*.o(.modinfo.module_info_suffix))
        link_module_info_suffix_end = .;
    }>APP_FLASH 

    .module_end : 
    {
        link_module_end = .;
    }>APP_FLASH 

    .module_info_crc :
    {
        link_module_info_crc_start = .;
        KEEP(*.o(.modinfo.module_info_crc))
        link_module_info_crc_end = .;
    }>APP_FLASH
