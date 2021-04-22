/* Declaration for the location of the application and system backup RAM region. */
/* FIXME: rtl872x has only 1KB of backup SRAM (Retention SRAM), we allocate 4KB out of normal SRAM */
BACKUPSRAM (rwx) : ORIGIN = _user_backup_ram_start, LENGTH = _user_backup_ram_size
BACKUPSRAM_SYSTEM (rwx) : ORIGIN = _system_backup_ram_start, LENGTH = _system_backup_ram_size
BACKUPSRAM_SYSTEM_FLAGS (rwx) : ORIGIN = _system_flags_ram_start, LENGTH = _system_flags_ram_size
BACKUPSRAM_REGISTERS (rwx) : ORIGIN = _backup_registers_ram_start, LENGTH = _backup_registers_ram_size

/* FIXME: This is only used in monolithic builds */
BACKUPSRAM_ALL (rwx) : ORIGIN = _system_backup_ram_start, LENGTH = _user_backup_ram_size + _system_backup_ram_size + _system_flags_ram_size + _backup_registers_ram_size
