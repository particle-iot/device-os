
#include "testapi.h"

test(adc_hal_backwards_compatibility)
{
    // These APIs are exposed to user application.
    API_COMPILE(HAL_ADC_Set_Sample_Time(0));
    API_COMPILE(HAL_ADC_Read(0));
}
