    .backup :
    {
        link_global_retained_initial_values = LOADADDR( .backup ); /* This is the location in flash of the initial values of retained global variables */
        link_global_retained_start = .;
        *(.retained_user*)
        link_global_retained_end = .;
        . = ALIGN(., 4);
    }>BACKUPSRAM AT> APP_FLASH

    .backup_system :
    {
        link_global_retained_system_start = .;
        *(.retained_system*)
        link_global_retained_system_end = .;
    }>BACKUPSRAM_SYSTEM AT> APP_FLASH
    /* even though we don't initialize the system backup RAM presently, it has to be
     placed at APP_FLASH or the resulting file ends up being hundreds of megabytes in size */
