/* Declaration for the location of the application and system backup RAM region. */
/* FIXME: nRF52840 does not have a dedicated backup SRAM. These were randomly selected */
BACKUPSRAM (rwx) : ORIGIN = _user_backup_ram_start, LENGTH = _user_backup_ram_size
BACKUPSRAM_SYSTEM (rwx) : ORIGIN = _system_backup_ram_start, LENGTH = _system_backup_ram_size
BACKUPSRAM_SYSTEM_FLAGS (rwx) : ORIGIN = _system_flags_ram_start, LENGTH = _system_flags_ram_size
BACKUPSRAM_REGISTERS (rwx) : ORIGIN = _backup_registers_ram_start, LENGTH = _backup_registers_ram_size

