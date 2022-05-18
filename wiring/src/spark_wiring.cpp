#include "spark_wiring.h"
#include "spark_wiring_ticks.h"
#include "adc_hal.h"
#include "system_task.h"


/*
 * @brief  Override the default ADC Sample time depending on requirement
 * @param  ADC_SampleTime: The sample time value to be set.
 *
 * @retval None
 */
void setADCSampleTime(uint8_t ADC_SampleTime)
{
    hal_adc_set_sample_time(ADC_SampleTime);
}

int map(int value, int fromStart, int fromEnd, int toStart, int toEnd)
{
    if (fromEnd == fromStart) {
        return value;
    }
    return (value - fromStart) * (toEnd - toStart) / (fromEnd - fromStart) + toStart;
}

double map(double value, double fromStart, double fromEnd, double toStart, double toEnd)
{
    if (fromEnd == fromStart) {
        return value;
    }
    return (value - fromStart) * (toEnd - toStart) / (fromEnd - fromStart) + toStart;
}

void delay(unsigned long ms)
{
    system_delay_ms(ms, false);
}
