
#include "hw_config.h"

int platform_watchdog_kick( void )
{
    /* Reload IWDG counter */
    IWDG_ReloadCounter( );
    return 0;
}

