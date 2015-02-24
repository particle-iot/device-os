
ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
ASSERT ( link_module_info_start - ORIGIN(FLASH) == 0x184,  "module info not where expected" )