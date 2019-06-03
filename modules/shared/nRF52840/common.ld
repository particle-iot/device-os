/* Stack */
_estack = _stack_end;
__Stack_Size = _stack_size;
__Stack_Init = _estack - __Stack_Size ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;
PROVIDE ( _Stack_Init = __Stack_Init ) ;
