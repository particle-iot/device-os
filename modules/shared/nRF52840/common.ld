
/* Stack */
_estack = 0x20000000 + 256K - LENGTH(BACKUPSRAM_ALL);
__Stack_Size = __STACKSIZE__;
__Stack_Init = _estack - __Stack_Size ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;
PROVIDE ( _Stack_Init = __Stack_Init ) ;


/* Minimal heap requirement */
min_heap_size = 32K;
