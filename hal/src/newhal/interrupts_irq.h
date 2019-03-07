#ifndef INTERRUPTS_IRQ_H
#define INTERRUPTS_IRQ_H

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef int32_t IRQn_Type;

typedef enum hal_irq_t {
    __Last_irq = 0
} hal_irq_t;

#ifdef  __cplusplus
}
#endif

#endif  /* INTERRUPTS_IRQ_H */
