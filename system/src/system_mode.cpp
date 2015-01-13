

#include "system_mode.h"
#include "spark_wlan.h"

static System_Mode_TypeDef _mode;

void set_system_mode(System_Mode_TypeDef mode) {
    _mode = mode;
    
    switch(mode)
    {
    case AUTOMATIC:
        SPARK_CLOUD_CONNECT = 1;
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

System_Mode_TypeDef system_mode() {
    return _mode;
}

