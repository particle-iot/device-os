
/* Definitions describing the static elements of this module */
/* These constants are used by the bootloader so should not be changed once the bootloader has been released. */


/* location of the module_info block */
mono_module_info = 0x8020184 ;

/**
 * End of the module info block.
 */
mono_module_info_end = mono_module_info + 24 ;