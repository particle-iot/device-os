/* Declaration for the location of the application and system backup RAM region. */
    BACKUPSRAM_USER         (rwx) : ORIGIN = platform_user_backup_ram_start,      LENGTH = platform_user_backup_ram_size
    BACKUPSRAM_SYSTEM_FLAGS (rwx) : ORIGIN = platform_system_flags_ram_start,     LENGTH = platform_system_flags_ram_size
    BACKUPSRAM_REGISTERS    (rwx) : ORIGIN = platform_backup_registers_ram_start, LENGTH = platform_backup_registers_ram_size
    BACKUPSRAM_SYSTEM       (rwx) : ORIGIN = platform_system_backup_ram_start,    LENGTH = platform_system_backup_ram_size

