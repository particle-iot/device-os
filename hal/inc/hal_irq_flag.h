#ifndef __HAL_IRQ_FLAG_H
#define __HAL_IRQ_FLAG_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
int HAL_disable_irq();
void HAL_enable_irq(int mask);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __HAL_IRQ_FLAG_H
