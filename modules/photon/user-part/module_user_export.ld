
user_module_table = 0x80A0000;

PROVIDE ( dynalib_location_user = user_module_table + 0 );

length_info = user_module_table + 4;