

#include "system_mode.h"
#include "system_task.h"

static System_Mode_TypeDef _mode = DEFAULT;

void set_system_mode(System_Mode_TypeDef mode)
{
    // the SystemClass is constructed twice, 
    // once for the `System` instance used by the system (to provide the System.xxx api)
    // and again for the SYSTEM_MODE() macro. The order of module initialization is
    // undefined in C++ so they may be initialized in any arbitrary order.
    // Meaning, the instance from SYSTEM_MODE() macro might be constructed first, 
    // followed by the `System` instance, which sets the mode back to `AUTOMATIC`.
    // The DEFAULT mode prevents this.
    if (mode==DEFAULT) {            // the default system instance
        if (_mode==DEFAULT)         // no mode set yet
            _mode = AUTOMATIC;      // set to automatic mode        
        else
            return;                 // don't change the current mode
    }
    
    _mode = mode;    
    switch (mode)
    {
        case SAFE_MODE:
        case AUTOMATIC:
            SPARK_CLOUD_CONNECT = 1;
            SPARK_WLAN_SLEEP = 0;
            break;

        case SEMI_AUTOMATIC:
            SPARK_CLOUD_CONNECT = 0;
            SPARK_WLAN_SLEEP = 1;
            break;

        case MANUAL:
            SPARK_CLOUD_CONNECT = 0;
            SPARK_WLAN_SLEEP = 1;
            break;                    
    }
}

System_Mode_TypeDef system_mode()
{
    return _mode;
}

