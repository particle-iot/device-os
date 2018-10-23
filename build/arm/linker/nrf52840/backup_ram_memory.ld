/* Declaration for the location of the application and system backup RAM region. */
/* FIXME: nRF52840 does not have a dedicated backup SRAM. These were randomly selected */
BACKUPSRAM (rwx) : ORIGIN = 0x20000000 + 256K - 3K, LENGTH = 3K
BACKUPSRAM_SYSTEM (rwx) : ORIGIN = 0x20000000 + 256K - 3K - 1K, LENGTH = 1K - 64 - 64
BACKUPSRAM_SYSTEM_FLAGS (rwx) : ORIGIN = 0x20000000 + 256K - 3K - 64, LENGTH = 64
BACKUPSRAM_REGISTERS (rwx) : ORIGIN = 0x20000000 + 256K - 3K - 64 - 64, LENGTH = 64
