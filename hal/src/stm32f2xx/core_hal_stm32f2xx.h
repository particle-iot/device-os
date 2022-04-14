#pragma once

#include "core_hal.h"
#include "hw_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set to non-zero value after RTOS initialization.
 */
extern volatile uint8_t rtos_started;

/**
 * Called by HAL_Core_Config() to setup SysTick_Configuration() if necessary.
 */
void HAL_Core_Config_systick_configuration(void);

/**
 * Called by HAL_Core_Config() to allow the HAL implementation to override
 * the interrupt table if required.
 */
void HAL_Core_Setup_override_interrupts(void);

/**
 * Called by HAL_Core_Setup() to perform any post-setup config after the
 * watchdog has been disabled.
 */
void HAL_Core_Setup_finalize(void);

/**
 * Called by HAL_Core_Init() to perform platform-specific HAL initialization before
 * entering the main loop and after user constructors have been called
 */
void HAL_Core_Init_finalize(void);

/**
 * The entrypoint to start system firmware and the application.
 * This should be called from the RTOS main thread once initialization has been
 * completed, constructors invoked and and HAL_Core_Config() has been called.
 */
void application_start(void);


/**
 * The HAL implementation should ensure that these interrupt handlers are hooked up
 * to their appropriate interrupt sources.
 */

void NMI_Handler(void);
void HardFault_Handler(void);
void UsageFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTickOverride(void);

void Mode_Button_EXTI_irq(void);
void HAL_USART1_Handler(void);
void HAL_USART2_Handler(void);
void HAL_USART3_Handler(void);
void HAL_USART4_Handler(void);
void HAL_USART5_Handler(void);
void ADC_irq();
void CAN1_TX_irq(void);
void CAN1_RX0_irq(void);
void CAN1_RX1_irq(void);
void CAN1_SCE_irq(void);
void TIM1_CC_irq(void);
void TIM2_irq(void);
void TIM3_irq(void);
void TIM4_irq(void);
void TIM5_irq(void);
void TIM6_DAC_irq(void);
void TIM7_override(void);
void TIM8_BRK_TIM12_irq(void);
void TIM8_UP_TIM13_irq(void);
void TIM8_TRG_COM_TIM14_irq(void);
void TIM8_CC_irq(void);
void TIM1_BRK_TIM9_irq(void);
void TIM1_UP_TIM10_irq(void);
void TIM1_TRG_COM_TIM11_irq(void);
void CAN2_TX_irq(void);
void CAN2_RX0_irq(void);
void CAN2_RX1_irq(void);
void CAN2_SCE_irq(void);
void I2C1_EV_irq(void);
void I2C1_ER_irq(void);
void I2C3_EV_irq(void);
void I2C3_ER_irq(void);
void DMA1_Stream7_irq(void);
void DMA2_Stream5_irq(void);
void DMA1_Stream2_irq(void);
void DMA2_Stream2_irq_override(void);

void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void RTC_Alarm_irq(void);


// etc... all ISRs ending _irq()). These are named after the values they had in WICED
// but they could easily be renamed using #defines if they need to conform to a different
// naming scheme.



/**
 * A shared handler for the EXTI interrupt to process presses of the mode button.
 */
void Handle_Mode_Button_EXTI_irq(Button_TypeDef button);

/**
 * Handle short and generic tasks for the device HAL on 1ms ticks
 */
void HAL_1Ms_Tick(void);

void HAL_EXTI_Handler(uint8_t EXTI_Line);

#ifdef __cplusplus
} // extern "C"
#endif
