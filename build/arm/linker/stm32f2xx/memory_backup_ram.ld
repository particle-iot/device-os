/* Declaration for the location of the application and system backup RAM region. */
    BACKUPSRAM        (rwx) : ORIGIN = platform_user_backup_ram_start,   LENGTH = platform_user_backup_ram_size
    BACKUPSRAM_SYSTEM (rwx) : ORIGIN = platform_system_backup_ram_start, LENGTH = platform_system_backup_ram_size
