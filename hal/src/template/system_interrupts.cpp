
#include "interrupts_hal.h"

uint8_t HAL_Set_System_Interrupt_Handler(hal_irq_t irq, const HAL_InterruptCallback* callback, HAL_InterruptCallback* previous, void* reserved)
{
    return false;
}

uint8_t HAL_Get_System_Interrupt_Handler(hal_irq_t irq, HAL_InterruptCallback* callback, void* reserved)
{
    return false;
}

void HAL_System_Interrupt_Trigger(hal_irq_t irq, void* reserved)
{
}
