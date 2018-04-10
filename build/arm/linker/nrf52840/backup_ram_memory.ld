/* Declaration for the location of the application and system backup RAM region. */
/* FIXME: nRF52840 does not have a dedicated backup SRAM. These were randomly selected */
    BACKUPSRAM (rwx) : ORIGIN = 0x20020000, LENGTH = 3K
    BACKUPSRAM_SYSTEM (rwx) : ORIGIN = 0x20021000-1K, LENGTH = 1K
