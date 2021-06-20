
#include "interrupts_hal.h"

/* For now, we remember only one handler, but in future this may be extended to a
 * dynamically linked list to allow for multiple handlers.
 */
static hal_interrupt_callback_t SystemInterruptHandlers[__Last_irq];


inline bool is_valid_irq(hal_irq_t irq) {
    return irq < __Last_irq;
}

uint8_t hal_interrupt_set_system_handler(hal_irq_t irq, const hal_interrupt_callback_t* callback, hal_interrupt_callback_t* previous, void* reserved) {
    if (!is_valid_irq(irq)) {
        return false;
    }
    hal_interrupt_callback_t& cb = SystemInterruptHandlers[irq];
    if (previous) {
        *previous = cb;
    }
    if (callback) {
        cb = *callback;
    }
    else {
        cb.handler = 0;
        cb.data = 0;
    }
    return true;
}

uint8_t hal_interrupt_get_system_handler(hal_irq_t irq, hal_interrupt_callback_t* callback, void* reserved) {
    if (!is_valid_irq(irq)) {
        return false;
    }

    if (callback) {
        hal_interrupt_callback_t& cb = SystemInterruptHandlers[irq];
        *callback = cb;
    }

    return true;
}

void hal_interrupt_trigger_system(hal_irq_t irq, void* reserved) {
    if (is_valid_irq(irq)) {
        hal_interrupt_callback_t& cb = SystemInterruptHandlers[irq];
        if (cb.handler) {
            cb.handler(cb.data);
        }
    }
}

