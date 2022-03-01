
#include "interrupts_hal.h"

/* For now, we remember only one handler, but in future this may be extended to a
 * dynamically linked list to allow for multiple handlers.
 */
static HAL_InterruptCallback SystemInterruptHandlers[__Last_irq];


inline bool is_valid_irq(hal_irq_t irq)
{
    return irq<__Last_irq;
}

uint8_t HAL_Set_System_Interrupt_Handler(hal_irq_t irq, const HAL_InterruptCallback* callback, HAL_InterruptCallback* previous, void* reserved)
{
    if (!is_valid_irq(irq))
        return false;
    HAL_InterruptCallback& cb = SystemInterruptHandlers[irq];
    if (previous)
        *previous = cb;
    if (callback)
        cb = *callback;
    else {
        cb.handler = 0;
        cb.data = 0;
    }

    return true;
}

uint8_t HAL_Get_System_Interrupt_Handler(hal_irq_t irq, HAL_InterruptCallback* callback, void* reserved)
{
    if (!is_valid_irq(irq))
        return false;

    if (callback) {
        HAL_InterruptCallback& cb = SystemInterruptHandlers[irq];
        *callback = cb;
    }

    return true;
}

void HAL_System_Interrupt_Trigger(hal_irq_t irq, void* reserved)
{
    if (is_valid_irq(irq))
    {
        HAL_InterruptCallback& cb = SystemInterruptHandlers[irq];
        if (cb.handler)
            cb.handler(cb.data);
    }
}

