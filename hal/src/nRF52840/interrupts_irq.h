#ifndef INTERRUPTS_IRQ_H
#define INTERRUPTS_IRQ_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef USE_STDPERIPH_DRIVER
#include "nrf52840.h"
#endif /* USE_STDPERIPH_DRIVER */

typedef enum hal_irq_t {
    __Last_irq = 0
} hal_irq_t;

#define IRQN_TO_IDX(irqn) ((int)irqn + 16)

#ifdef  __cplusplus
}
#endif

#endif  /* INTERRUPTS_IRQ_H */
