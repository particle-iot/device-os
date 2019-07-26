/* Declaration for the location of the application and system backup RAM region. */
    BACKUPSRAM        (rwx) : ORIGIN = _user_backup_ram_start,   LENGTH = _user_backup_ram_size
    BACKUPSRAM_SYSTEM (rwx) : ORIGIN = _system_backup_ram_start, LENGTH = _system_backup_ram_size
