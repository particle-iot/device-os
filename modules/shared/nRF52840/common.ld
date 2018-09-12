
/* Backup RAM */
user_backup_ram_size = 3K;
system_backup_ram_size = 1K;


/* Stack */
_estack = 0x20000000 + 256K - user_backup_ram_size - system_backup_ram_size;
__Stack_Size = 4096 ;
__Stack_Init = _estack - __Stack_Size ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;
PROVIDE ( _Stack_Init = __Stack_Init ) ;


/* Minimal heap requirement */
min_heap_size = 32K;
