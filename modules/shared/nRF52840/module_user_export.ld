/* Assume the user static RAM start address */
user_static_ram_size = 96K;
user_static_ram_start = 0x20000000 + 256K - LENGTH(BACKUPSRAM_ALL) - __Stack_Size - user_static_ram_size;


/* location of the module_info block */
user_module_info = 0xD4000 ;

/**
 * End of the module info block.
 */
user_module_info_end = user_module_info + 24 ;
user_module_table = user_module_info_end ;

PROVIDE ( dynalib_location_user = user_module_table + 0 );

