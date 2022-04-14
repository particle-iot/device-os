/* location of the module_info block */
user_module_info = 0xB4000 ;

/**
 * End of the module info block.
 */
user_module_info_end = user_module_info + 24 ;
user_module_table = user_module_info_end ;

/* location of the module_info block of a compat 128KB user binary */
user_module_info_compat = 0xD4000 ;

/**
 * End of the compat module info block.
 */
user_module_info_end_compat = user_module_info_compat + 24 ;
user_module_table_compat = user_module_info_end_compat ;


PROVIDE ( dynalib_location_user = user_module_table + 0 );
PROVIDE ( dynalib_location_user_compat = user_module_table_compat + 0 );
