
#include "hw_config.h"

// This is required by the WICED DCT functions
int platform_watchdog_kick( void )
{
    /* Reload IWDG counter */
    IWDG_ReloadCounter( );
    return 0;
}

