
/* Definitions describing the static elements of this module */
/* These constants are used by the bootloader so should not be changed once the bootloader has been released. */


/* location of the module_info block */
mono_module_info = APP_CODE_BASE + 0x200;

/**
 * End of the module info block.
 */
mono_module_info_end = mono_module_info + 24 ;