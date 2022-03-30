/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

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

void HAL_Core_Restore_Interrupt(IRQn_Type irqn);
#ifdef  __cplusplus
}
#endif

#endif  /* INTERRUPTS_IRQ_H */
