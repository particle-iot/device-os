    /* NOTE: this section will be stripped out of the resulting binary */
    .module_info_product :
    {
        link_module_info_static_start = .;
        FILL(0xffff)
        . = link_module_info_static_start + 2;
        KEEP (*.o(.modinfo.product_id))
        . = link_module_info_static_start + 4;
        FILL(0xffff)
        KEEP (*.o(.modinfo.product_version))
        . = link_module_info_static_start + 4 + 2;
    } > APP_FLASH

    .module_info_suffix :
    {
        link_module_info_suffix_start = .;
        KEEP (*.o(.modinfo.module_info_suffix))
        link_module_info_suffix_end = .;
    } > APP_FLASH

    .module_end :
    {
        link_module_end = .;
    } > APP_FLASH

    .module_info_crc :
    {
        link_module_info_crc_start = .;
        KEEP (*.o(.modinfo.module_info_crc))
        link_module_info_crc_end = .;
    } > APP_FLASH
