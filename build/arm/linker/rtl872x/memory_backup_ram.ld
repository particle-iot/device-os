/* Declaration for the location of the application and system backup RAM region. */
/* FIXME: rtl872x has only 1KB of backup SRAM (Retention SRAM), we allocate 4KB out of normal SRAM
 *        KM0 and KM4 share the same backup memory. */
    BACKUPSRAM_USER         (rwx) : ORIGIN = platform_user_backup_ram_start,      LENGTH = platform_user_backup_ram_size
    BACKUPSRAM_SYSTEM_FLAGS (rwx) : ORIGIN = platform_system_flags_ram_start,     LENGTH = platform_system_flags_ram_size
    BACKUPSRAM_REGISTERS    (rwx) : ORIGIN = platform_backup_registers_ram_start, LENGTH = platform_backup_registers_ram_size
    BACKUPSRAM_SYSTEM       (rwx) : ORIGIN = platform_system_backup_ram_start,    LENGTH = platform_system_backup_ram_size
