#pragma once


/**
 * Called by HAL_Core_Config() to allow the HAL implementation to override
 * the interrupt table if required.
 */
void HAL_Core_Setup_override_interrupts();

/**
 * Called by HAL_Core_Setup() to perform any post-setup config after the
 * watchdog has been disabled.
 */
void HAL_Core_Setup_finalize();


/**
 * The entrypoint to start system firmware and the application.
 * This should be called from the RTOS main thread once initialization has been
 * completed, constructors invoked and and HAL_Core_Config() has been called.
 */
void application_start();


/**
 * The HAL implementation should ensure that these interrupt handlers are hooked up
 * to their appropriate interrupt sources.
 */

void SysTickOverride(void);
void Mode_Button_EXTI_irq(void);
void HAL_USART1_Handler(void);
void HardFault_Handler(void);
void UsageFault_Handler(void);
void ADC_irq();
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
void CAN2_TX_irq();
void CAN2_RX0_irq();
void CAN2_RX1_irq();
void CAN2_SCE_irq();
// etc... all ISRs ending _irq()). These are named after the values they had in WICED
// but they could easily be renamed using #defines if they need to conform to a different
// naming scheme.

/**
 * A shared handler for the EXTI interrupt to process presses of the mode button.
 */
void Handle_Mode_Button_EXTI_irq(void);

/**
 * Handle short and generic tasks for the device HAL on 1ms ticks
 */
void HAL_1Ms_Tick(void);

