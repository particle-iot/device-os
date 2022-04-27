
#include "testapi.h"

test(adc_hal_backwards_compatibility)
{
    // These APIs are exposed to user application.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    API_COMPILE(HAL_ADC_Set_Sample_Time(0));
    API_COMPILE(HAL_ADC_Read(0));
#pragma GCC diagnostic pop
}
