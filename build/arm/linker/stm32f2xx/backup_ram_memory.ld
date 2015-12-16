/* Declaration for the location of the application and system backup RAM region. */
    BACKUPSRAM (rwx) : ORIGIN = 0x40024000, LENGTH = 3K
    BACKUPSRAM_SYSTEM (rwx) : ORIGIN = 0x40025000-1K, LENGTH = 1K
