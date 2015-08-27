/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Satish Nair, Matthew McGowan
 * @version V1.0.0
 * @date    31-Oct-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include <stdint.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "interrupts_impl.h"
#include "ota_flash_hal.h"
#include "rtc_hal.h"
#include "rng_hal.h"
#include "syshealth_hal.h"
#include "rgbled.h"
#include "delay_hal.h"
#include "hw_config.h"
#include "service_debug.h"
#include "flash_mal.h"
#include <stdatomic.h>
#include "stm32f2xx.h"
#include "core_cm3.h"
#include "bootloader.h"
#include "core_hal_stm32f2xx.h"

void HardFault_Handler( void ) __attribute__( ( naked ) );

__attribute__((externally_visible)) void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    /* These are volatile to try and prevent the compiler/linker optimising them
    away as the variables never actually get used.  If the debugger won't show the
    values of the variables, make them global my moving their declaration outside
    of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr; /* Link register. */
    volatile uint32_t pc; /* Program counter. */
    volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    if (false)
        r0++; r1++; r2++; r3++; r12++; lr++; pc++; psr++;


    if (SCB->CFSR & (1<<25) /* DIVBYZERO */) {
        // stay consistent with the core and cause 5 flashes
        UsageFault_Handler();
    }
    else {
    PANIC(HardFault,"HardFault");

    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}
}


void HardFault_Handler(void)
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

/*******************************************************************************
 * Function Name  : MemManage_Handler
 * Description    : This function handles Memory Manage exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
        PANIC(MemManage,"MemManage");
	while (1)
	{
	}
}

/*******************************************************************************
 * Function Name  : BusFault_Handler
 * Description    : This function handles Bus Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
        PANIC(BusFault,"BusFault");
        while (1)
	{
	}
}


/*******************************************************************************
 * Function Name  : UsageFault_Handler
 * Description    : This function handles Usage Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
        PANIC(UsageFault,"UsageFault");
	while (1)
	{
	}
}



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void (*HAL_TIM1_Handler)(void);
void (*HAL_TIM3_Handler)(void);
void (*HAL_TIM4_Handler)(void);
void (*HAL_TIM5_Handler)(void);
void (*HAL_TIM8_Handler)(void);

/* Extern variables ----------------------------------------------------------*/
extern __IO uint16_t BUTTON_DEBOUNCED_TIME[];


/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
{
    DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);

#ifdef DFU_BUILD_ENABLE
    //Currently this is done through WICED library API so commented.
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x20000);
    USE_SYSTEM_FLAGS = 1;
#endif

    Set_System();

    //Wiring pins default to inputs
#if !defined(USE_SWD_JTAG) && !defined(USE_SWD)
    for (pin_t pin=0; pin<=19; pin++)
        HAL_Pin_Mode(pin, INPUT);
#if PLATFORM_ID==8 // Additional pins for P1
    for (pin_t pin=24; pin<=29; pin++)
        HAL_Pin_Mode(pin, INPUT);
#endif
#if PLATFORM_ID==10 // Additional pins for Electron
    for (pin_t pin=24; pin<=35; pin++)
        HAL_Pin_Mode(pin, INPUT);
#endif
#endif

    HAL_Core_Config_systick_configuration();

    //HAL_RTC_Configuration();

    HAL_RNG_Configuration();

#ifdef DFU_BUILD_ENABLE
    Load_SystemFlags();
#endif

    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);

    // override the WICED interrupts, specifically SysTick - there is a bug
    // where WICED isn't ready for a SysTick until after main() has been called to
    // fully intialize the RTOS.
    HAL_Core_Setup_override_interrupts();

#if MODULAR_FIRMWARE
    // write protect system module parts if not already protected
    FLASH_WriteProtectMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION - CORE_FW_ADDRESS, true);
#endif

#ifdef HAS_SERIAL_FLASH
    //Initialize Serial Flash
    sFLASH_Init();
#else
    FLASH_AddToFactoryResetModuleSlot(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS,
                                      FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE,
                                      FACTORY_RESET_MODULE_FUNCTION, MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS); //true to verify the CRC during copy also
#endif


}

#if !MODULAR_FIRMWARE
__attribute__((externally_visible, section(".early_startup.HAL_Core_Config"))) uint32_t startup = (uint32_t)&HAL_Core_Config;
#endif

void HAL_Core_Setup(void) {

    /* Reset system to disable IWDG if enabled in bootloader */
    IWDG_Reset_Enable(0);

    HAL_Core_Setup_finalize();

    bootloader_update_if_needed();

}

#if MODULAR_FIRMWARE

bool HAL_Core_Validate_User_Module(void)
{
    bool valid = false;

    if (!SYSTEM_FLAG(StartupMode_SysFlag) & 1)
    {
        //CRC verification Enabled by default
        if (FLASH_isUserModuleInfoValid(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, USER_FIRMWARE_IMAGE_LOCATION))
        {
            //CRC check the user module and set to module_user_part_validated
            valid = FLASH_VerifyCRC32(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION,
                                         FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION))
                    && HAL_Verify_User_Dependencies();
        }
        else if(FLASH_isUserModuleInfoValid(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION))
        {
            //Reset and let bootloader perform the user module factory reset
            //Doing this instead of calling FLASH_RestoreFromFactoryResetModuleSlot()
            //saves precious system_part2 flash size i.e. fits in < 128KB
            HAL_Core_Factory_Reset();

            while(1);//Device should reset before reaching this line
        }
    }
    return valid;
}
#endif


bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    bool pressedState = false;

    if(BUTTON_GetDebouncedTime(BUTTON1) >= pressedMillisDuration)
    {
        pressedState = true;
    }

    return pressedState;
}

void HAL_Core_Mode_Button_Reset(void)
{
    BUTTON_ResetDebouncedState(BUTTON1);
}

void HAL_Core_System_Reset(void)
{
    NVIC_SystemReset();
}

void HAL_Core_Factory_Reset(void)
{
    system_flags.Factory_Reset_SysFlag = 0xAAAA;
    Save_SystemFlags();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Bootloader(bool persist)
{
    if (persist)
    {
        RTC_WriteBackupRegister(RTC_BKP_DR10, 0xFFFF);
        system_flags.FLASH_OTA_Update_SysFlag = 0xFFFF;
        Save_SystemFlags();
    }
    else
    {
        RTC_WriteBackupRegister(RTC_BKP_DR1, ENTER_DFU_APP_REQUEST);
    }

    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
    if ((wakeUpPin < TOTAL_PINS) && (edgeTriggerMode <= FALLING))
    {
        PinMode wakeUpPinMode = INPUT;

        /* Set required pinMode based on edgeTriggerMode */
        switch(edgeTriggerMode)
        {
        case CHANGE:
            wakeUpPinMode = INPUT;
            break;

        case RISING:
            wakeUpPinMode = INPUT_PULLDOWN;
            break;

        case FALLING:
            wakeUpPinMode = INPUT_PULLUP;
            break;
        }
        HAL_Pin_Mode(wakeUpPin, wakeUpPinMode);

        /* Configure EXTI Interrupt : wake-up from stop mode using pin interrupt */
        HAL_Interrupts_Attach(wakeUpPin, NULL, NULL, edgeTriggerMode, NULL);

        HAL_Core_Execute_Stop_Mode();

        /* Detach the Interrupt pin */
        HAL_Interrupts_Detach(wakeUpPin);
    }
}

void HAL_Core_Execute_Stop_Mode(void)
{
    /* Enable WKUP pin */
    PWR_WakeUpPinCmd(ENABLE);

    /* Request to enter STOP mode with regulator in low power mode */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

    /* At this stage the system has resumed from STOP mode */
    /* Enable HSE, PLL and select PLL as system clock source after wake-up from STOP */

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    if(RCC_WaitForHSEStartUp() != SUCCESS)
    {
        /* If HSE startup fails try to recover by system reset */
        NVIC_SystemReset();
    }

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08);
}

void HAL_Core_Enter_Standby_Mode(void)
{
    HAL_Core_Execute_Standby_Mode();
}

void HAL_Core_Execute_Standby_Mode(void)
{
    /* Enable WKUP pin */
    PWR_WakeUpPinCmd(ENABLE);

    /* Request to enter STANDBY mode */
    PWR_EnterSTANDBYMode();

    /* Following code will not be reached */
    while(1);
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
    return Compute_CRC32(pBuffer, bufferSize);
}

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag)
{
    switch (flag) {
        case BOOTLOADER_FLAG_VERSION:
            return SYSTEM_FLAG(Bootloader_Version_SysFlag);
        case BOOTLOADER_FLAG_STARTUP_MODE:
            return SYSTEM_FLAG(StartupMode_SysFlag);
    }
    return 0;
}

/**
 * The entrypoint to our application.
 * This should be called from the RTOS main thread once initialization has been
 * completed, constructors invoked and and HAL_Core_Config() has been called.
 */
void application_start()
{
    // one the key is sent to the cloud, this can be removed, since the key is fetched in
    // Spark_Protocol_init(). This is just a temporary measure while the key still needs
    // to be fetched via DFU.

    HAL_Core_Setup();

    // normallly allocating such a large buffer on the stack would be a bad idea, however, we are quite near the start of execution, with few levels of recursion.
    char buf[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];
    // ensure the private key is provisioned

    // Reset the system after generating the key - reports of Serial not being available in listening mode
    // after generating the key.
    private_key_generation_t genspec;
    genspec.size = sizeof(genspec);
    genspec.gen = PRIVATE_KEY_GENERATE_MISSING;
    HAL_FLASH_Read_CorePrivateKey(buf, &genspec);
    if (genspec.generated_key)
        HAL_Core_System_Reset();


    app_setup_and_loop();
}


#ifdef UNUSED
#undef UNUSED
#endif
#define UNUSED(x) (void)(x)

uint8_t handle_timer(TIM_TypeDef* TIMx, uint16_t TIM_IT, hal_irq_t irq)
{
    uint8_t result = (TIM_GetITStatus(TIMx, TIM_IT)!=RESET);
    if (result) {
        HAL_System_Interrupt_Trigger(irq, NULL);
        TIM_ClearITPendingBit(TIMx, TIM_IT);
    }
    return result;
}


/**
 * The following tick hook will only get called if configUSE_TICK_HOOK
 * is set to 1 within FreeRTOSConfig.h
 */
void SysTickOverride(void)
{
    System1MsTick();

    /* Handle short and generic tasks for the device HAL on 1ms ticks */
    HAL_1Ms_Tick();

    HAL_SysTick_Handler();

    HAL_System_Interrupt_Trigger(SysInterrupt_SysTick, NULL);
}

/**
 * @brief  This function handles EXTI2_IRQ or EXTI_9_5_IRQ Handler.
 * @param  None
 * @retval None
 */
void Handle_Mode_Button_EXTI_irq(void)
{
    if (EXTI_GetITStatus(BUTTON1_EXTI_LINE) != RESET)
    {
        /* Clear the EXTI line pending bit (cleared in WICED GPIO IRQ handler) */
        EXTI_ClearITPendingBit(BUTTON1_EXTI_LINE);

        BUTTON_DEBOUNCED_TIME[BUTTON1] = 0x00;

        /* Disable BUTTON1 Interrupt */
        BUTTON_EXTI_Config(BUTTON1, DISABLE);

        /* Enable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    }

}


void ADC_irq()
{
    HAL_System_Interrupt_Trigger(SysInterrupt_ADC_IRQ, NULL);
}

/**
 * @brief  This function handles TIM1_CC_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM1_CC_irq(void)
{
    if(NULL != HAL_TIM1_Handler)
    {
        HAL_TIM1_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_CC_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM1, TIM_IT_CC1, SysInterrupt_TIM1_Compare1) ||
    handle_timer(TIM1, TIM_IT_CC2, SysInterrupt_TIM1_Compare2) ||
    handle_timer(TIM1, TIM_IT_CC3, SysInterrupt_TIM1_Compare3) ||
    handle_timer(TIM1, TIM_IT_CC4, SysInterrupt_TIM1_Compare4);
    UNUSED(result);
}

/**
 * @brief  This function handles TIM2_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM2_irq(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
        {
            BUTTON_DEBOUNCED_TIME[BUTTON1] += BUTTON_DEBOUNCE_INTERVAL;
        }
        else
        {
            /* Disable TIM2 CC1 Interrupt */
            TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);

            /* Enable BUTTON1 Interrupt */
            BUTTON_EXTI_Config(BUTTON1, ENABLE);
        }
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM2_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM2, TIM_IT_CC1, SysInterrupt_TIM2_Compare1) ||
    handle_timer(TIM2, TIM_IT_CC2, SysInterrupt_TIM2_Compare2) ||
    handle_timer(TIM2, TIM_IT_CC3, SysInterrupt_TIM2_Compare4) ||
    handle_timer(TIM2, TIM_IT_CC4, SysInterrupt_TIM2_Compare3) ||
    handle_timer(TIM2, TIM_IT_Update, SysInterrupt_TIM2_Update) ||
    handle_timer(TIM2, TIM_IT_Trigger, SysInterrupt_TIM2_Trigger);
    UNUSED(result);
}

/**
 * @brief  This function handles TIM3_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM3_irq(void)
{
    if(NULL != HAL_TIM3_Handler)
    {
        HAL_TIM3_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM3_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM3, TIM_IT_CC1, SysInterrupt_TIM3_Compare1) ||
    handle_timer(TIM3, TIM_IT_CC2, SysInterrupt_TIM3_Compare2) ||
    handle_timer(TIM3, TIM_IT_CC3, SysInterrupt_TIM3_Compare4) ||
    handle_timer(TIM3, TIM_IT_CC4, SysInterrupt_TIM3_Compare3) ||
    handle_timer(TIM3, TIM_IT_Update, SysInterrupt_TIM3_Update) ||
    handle_timer(TIM3, TIM_IT_Trigger, SysInterrupt_TIM3_Trigger);
    UNUSED(result);
}

/**
 * @brief  This function handles TIM4_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM4_irq(void)
{
    if(NULL != HAL_TIM4_Handler)
    {
        HAL_TIM4_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM4_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM4, TIM_IT_CC1, SysInterrupt_TIM4_Compare1) ||
    handle_timer(TIM4, TIM_IT_CC2, SysInterrupt_TIM4_Compare2) ||
    handle_timer(TIM4, TIM_IT_CC3, SysInterrupt_TIM4_Compare4) ||
    handle_timer(TIM4, TIM_IT_CC4, SysInterrupt_TIM4_Compare3) ||
    handle_timer(TIM4, TIM_IT_Update, SysInterrupt_TIM4_Update) ||
    handle_timer(TIM4, TIM_IT_Trigger, SysInterrupt_TIM4_Trigger);
    UNUSED(result);

}

/**
 * @brief  This function handles TIM5_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM5_irq(void)
{
    if(NULL != HAL_TIM5_Handler)
    {
        HAL_TIM5_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM5_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM5, TIM_IT_CC1, SysInterrupt_TIM5_Compare1) ||
    handle_timer(TIM5, TIM_IT_CC2, SysInterrupt_TIM5_Compare2) ||
    handle_timer(TIM5, TIM_IT_CC3, SysInterrupt_TIM5_Compare4) ||
    handle_timer(TIM5, TIM_IT_CC4, SysInterrupt_TIM5_Compare3) ||
    handle_timer(TIM5, TIM_IT_Update, SysInterrupt_TIM5_Update) ||
    handle_timer(TIM5, TIM_IT_Trigger, SysInterrupt_TIM5_Trigger);
    UNUSED(result);
}

void TIM6_DAC_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM6_DAC_IRQ, NULL);
    handle_timer(TIM6, TIM_IT_Update, SysInterrupt_TIM6_Update);
}

void TIM7_override(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM7_IRQ, NULL);
    handle_timer(TIM7, TIM_IT_Update, SysInterrupt_TIM7_Update);
}

void TIM8_BRK_TIM12_irq(void)
{

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_BRK_TIM12_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_Break, SysInterrupt_TIM8_Break) ||
    handle_timer(TIM12, TIM_IT_CC1, SysInterrupt_TIM12_Compare1) ||
    handle_timer(TIM12, TIM_IT_CC2, SysInterrupt_TIM12_Compare2) ||
    handle_timer(TIM12, TIM_IT_Update, SysInterrupt_TIM12_Update) ||
    handle_timer(TIM12, TIM_IT_Trigger, SysInterrupt_TIM12_Trigger);
    UNUSED(result);
}

void TIM8_UP_TIM13_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_UP_TIM13_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_Update, SysInterrupt_TIM8_Update) ||
    handle_timer(TIM13, TIM_IT_CC1, SysInterrupt_TIM13_Compare) ||
    handle_timer(TIM13, TIM_IT_Update, SysInterrupt_TIM13_Update);
    UNUSED(result);
}

void TIM8_TRG_COM_TIM14_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_TRG_COM_TIM14_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_Trigger, SysInterrupt_TIM8_Trigger) ||
    handle_timer(TIM14, TIM_IT_COM, SysInterrupt_TIM14_COM) ||
    handle_timer(TIM14, TIM_IT_CC1, SysInterrupt_TIM14_Compare);
    handle_timer(TIM14, TIM_IT_Update, SysInterrupt_TIM14_Update);
    UNUSED(result);
}

void TIM8_CC_irq(void)
{
#if PLATFORM_ID == 10 // Electron
    if(NULL != HAL_TIM8_Handler)
    {
        HAL_TIM8_Handler();
    }
#endif

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_CC1, SysInterrupt_TIM8_Compare1) ||
    handle_timer(TIM8, TIM_IT_CC2, SysInterrupt_TIM8_Compare2) ||
    handle_timer(TIM8, TIM_IT_CC3, SysInterrupt_TIM8_Compare3) ||
    handle_timer(TIM8, TIM_IT_CC4, SysInterrupt_TIM8_Compare4);
    UNUSED(result);
}

void TIM1_BRK_TIM9_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_BRK_TIM9_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM1, TIM_IT_Break, SysInterrupt_TIM1_Break) ||
    handle_timer(TIM9, TIM_IT_CC1, SysInterrupt_TIM9_Compare1) ||
    handle_timer(TIM9, TIM_IT_CC2, SysInterrupt_TIM9_Compare2) ||
    handle_timer(TIM9, TIM_IT_Update, SysInterrupt_TIM9_Update) ||
    handle_timer(TIM9, TIM_IT_Trigger, SysInterrupt_TIM9_Trigger);
    UNUSED(result);
}

void TIM1_UP_TIM10_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_UP_TIM10_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM1, TIM_IT_Update, SysInterrupt_TIM1_Update) ||
    handle_timer(TIM10, TIM_IT_CC1, SysInterrupt_TIM10_Compare) ||
    handle_timer(TIM10, TIM_IT_Update, SysInterrupt_TIM10_Update);
    UNUSED(result);
}

void TIM1_TRG_COM_TIM11_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_TRG_COM_TIM11_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM1, TIM_IT_Trigger, SysInterrupt_TIM1_Trigger) ||
    handle_timer(TIM1, TIM_IT_COM, SysInterrupt_TIM1_COM) ||
    handle_timer(TIM11, TIM_IT_CC1, SysInterrupt_TIM11_Compare) ||
    handle_timer(TIM11, TIM_IT_Update, SysInterrupt_TIM11_Update);
    UNUSED(result);
}

void CAN2_TX_irq()
{
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_TX_IRQ, NULL);
}

void CAN2_RX0_irq()
{
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_RX0_IRQ, NULL);
}

void CAN2_RX1_irq()
{
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_RX1_IRQ, NULL);
}

void CAN2_SCE_irq()
{
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_SCE_IRQ, NULL);
}

void HAL_Bootloader_Lock(bool lock)
{
    if (lock)
        FLASH_WriteProtection_Enable(BOOTLOADER_FLASH_PAGES);
    else
        FLASH_WriteProtection_Disable(BOOTLOADER_FLASH_PAGES);
}

bool HAL_Core_System_Reset_FlagSet(RESET_TypeDef resetType)
{
    uint8_t FLAG_Mask = 0x1F;
    uint8_t RCC_Flag = 0;

    switch(resetType)
    {
    case PIN_RESET:
        RCC_Flag = RCC_FLAG_PINRST;
        break;

    case SOFTWARE_RESET:
        RCC_Flag = RCC_FLAG_SFTRST;
        break;

    case WATCHDOG_RESET:
        RCC_Flag = RCC_FLAG_IWDGRST;
        break;

    case LOW_POWER_RESET:
        RCC_Flag = RCC_FLAG_LPWRRST;
        break;
    }

    if ((RCC_Flag != 0) && (SYSTEM_FLAG(RCC_CSR_SysFlag) & ((uint32_t)1 << (RCC_Flag & FLAG_Mask))))
    {
        return true;
    }

    return false;
}

