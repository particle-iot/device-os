/**
 ******************************************************************************
 * @file    hw_config.c
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Hardware Configuration & Setup
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include <string.h>
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define DBGMCU_CR_SETTINGS      (DBGMCU_CR_DBG_SLEEP|DBGMCU_CR_DBG_STOP|DBGMCU_CR_DBG_STANDBY)
#define DBGMCU_APB1FZ_SETTINGS  (DBGMCU_APB1_FZ_DBG_IWDG_STOP|DBGMCU_APB1_FZ_DBG_WWDG_STOP)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t USE_SYSTEM_FLAGS = 0;	//0, 1
uint16_t sys_health_cache = 0; // Used by the SYS_HEALTH macros store new heath if higher

GPIO_TypeDef* LED_GPIO_PORT[] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT, LED4_GPIO_PORT};
const uint16_t LED_GPIO_PIN[] = {LED1_GPIO_PIN, LED2_GPIO_PIN, LED3_GPIO_PIN, LED4_GPIO_PIN};
const uint32_t LED_GPIO_CLK[] = {LED1_GPIO_CLK, LED2_GPIO_CLK, LED3_GPIO_CLK, LED4_GPIO_CLK};
const uint8_t LED_GPIO_PIN_SOURCE[] = {LED1_GPIO_PIN_SOURCE, LED2_GPIO_PIN_SOURCE, LED3_GPIO_PIN_SOURCE, LED4_GPIO_PIN_SOURCE};
const uint8_t LED_GPIO_AF_TIM[] = {LED1_GPIO_AF_TIM, LED2_GPIO_AF_TIM, LED3_GPIO_AF_TIM, LED4_GPIO_AF_TIM};

GPIO_TypeDef* BUTTON_GPIO_PORT[] = {BUTTON1_GPIO_PORT};
const uint16_t BUTTON_GPIO_PIN[] = {BUTTON1_GPIO_PIN};
const uint32_t BUTTON_GPIO_CLK[] = {BUTTON1_GPIO_CLK};
GPIOMode_TypeDef BUTTON_GPIO_MODE[] = {BUTTON1_GPIO_MODE};
GPIOPuPd_TypeDef BUTTON_GPIO_PUPD[] = {BUTTON1_GPIO_PUPD};
__IO uint16_t BUTTON_DEBOUNCED_TIME[] = {0};

const uint16_t BUTTON_EXTI_LINE[] = {BUTTON1_EXTI_LINE};
const uint16_t BUTTON_EXTI_PORT_SOURCE[] = {BUTTON1_EXTI_PORT_SOURCE};
const uint16_t BUTTON_EXTI_PIN_SOURCE[] = {BUTTON1_EXTI_PIN_SOURCE};
const uint16_t BUTTON_IRQn[] = {BUTTON1_EXTI_IRQn};
EXTITrigger_TypeDef BUTTON_EXTI_TRIGGER[] = {BUTTON1_EXTI_TRIGGER};

uint16_t CORE_FW_Version_SysFlag = 0xFFFF;
uint16_t NVMEM_SPARK_Reset_SysFlag = 0xFFFF;
uint16_t FLASH_OTA_Update_SysFlag = 0xFFFF;
uint16_t OTA_FLASHED_Status_SysFlag = 0xFFFF;
uint16_t Factory_Reset_SysFlag = 0xFFFF;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Initialise Data Watchpoint and Trace Register (DWT).
 * @param  None
 * @retval None
 */
static void DWT_Init(void)
{
    DBGMCU->CR |= DBGMCU_CR_SETTINGS;
    DBGMCU->APB1FZ |= DBGMCU_APB1FZ_SETTINGS;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Configures Main system clocks & power.
 * @param  None
 * @retval None
 */
void Set_System(void)
{
    /*!< At this stage the microcontroller clock setting is already configured,
	 this is done through SystemInit() function which is called from startup
	 file (startup_stm32f2xx.S) before to branch to application main.
	 To reconfigure the default setting of SystemInit() function, refer to
	 system_stm32f2xx.c file
     */

    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* Allow access to RTC Backup domain */
    PWR_BackupAccessCmd(ENABLE);

    /* Should we execute System Standby mode */
    // Use "HAL_Core_Execute_Standby_Mode()" defined in core_hal.c
    // Or Use below code
    if(RTC_ReadBackupRegister(RTC_BKP_DR9) == 0xA5A5)
    {
        /* Clear Standby mode system flag */
        RTC_WriteBackupRegister(RTC_BKP_DR9, 0xFFFF);

        /* Request to enter STANDBY mode */
        PWR_EnterSTANDBYMode();

        /* Following code will not be reached */
        while(1);
    }

    DWT_Init();

    /* NVIC configuration */
    NVIC_Configuration();

    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* Configure TIM2 for LED-PWM and BUTTON-DEBOUNCE usage */
    UI_Timer_Configure();

    /* Configure the LEDs and set the default states */
    int LEDx;
    for(LEDx = 0; LEDx < LEDn; ++LEDx)
    {
        LED_Init(LEDx);
    }

    /* Configure the Button */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);
}

/*******************************************************************************
 * Function Name  : NVIC_Configuration
 * Description    : Configures Vector Table base location.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void NVIC_Configuration(void)
{
    /* Configure the NVIC Preemption Priority Bits */
    /* 4 bits for pre-emption priority(0-15 PreemptionPriority) and 0 bits for subpriority(0 SubPriority) */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* Configure the Priority Group to 2 bits */
    /* 2 bits for pre-emption priority(0-3 PreemptionPriority) and 2 bits for subpriority(0-3 SubPriority) */
    //OLD: NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Setup SysTick Timer and Configure its Interrupt Priority
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SysTick_Configuration(void)
{
    /* Setup SysTick Timer for 1 msec interrupts */
    if (SysTick_Config(SystemCoreClock / 1000))
    {
        /* Capture error */
        while (1)
        {
        }
    }

    /* Configure the SysTick Handler Priority: Preemption priority and subpriority */
    NVIC_SetPriority(SysTick_IRQn, SYSTICK_IRQ_PRIORITY);	//OLD: NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x03, 0x00)
}

void RTC_Configuration(void)
{
    //To Do
}

void IWDG_Reset_Enable(uint32_t msTimeout)
{
    //To Do
}

void UI_Timer_Configure(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* Enable TIM2 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* TIM2 Update Frequency = 120000000/120/10000 = 100Hz = 10ms */
    /* TIM2_Prescaler: 120 */
    /* TIM2_Autoreload: 9999 -> 100Hz = 10ms */
    uint16_t TIM2_Prescaler = SystemCoreClock / 1000000;
    uint16_t TIM2_Autoreload = (1000000 / UI_TIMER_FREQUENCY) - 1;

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    /* Time Base Configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = TIM2_Autoreload;
    TIM_TimeBaseStructure.TIM_Prescaler = TIM2_Prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0000;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCStructInit(&TIM_OCInitStructure);

    /* Output Compare Timing Mode configuration: Channel 1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0x0000;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);

    /* PWM1 Mode configuration: Channel 2, 3 and 4 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0x0000;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Disable);

    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Disable);

    TIM_OC4Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Disable);

    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* TIM2 enable counter */
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief  Configures LED GPIO.
 * @param  Led: Specifies the Led to be configured.
 *   This parameter can be one of following parameters:
 *     @arg LED1, LED2, LED3, LED4
 * @retval None
 */
void LED_Init(Led_TypeDef Led)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO_LED Clock */
    RCC_AHB1PeriphClockCmd(LED_GPIO_CLK[Led], ENABLE);

    /* Configure the GPIO_LED pins, mode, speed etc. */
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN[Led];
    if(Led == LED_USER)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    }
    else
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    }
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(LED_GPIO_PORT[Led], &GPIO_InitStructure);

    if(Led == LED_USER)
    {
        Set_User_LED(DISABLE);
    }
    else
    {
        /* Connect TIM pins to respective AF */
        GPIO_PinAFConfig(LED_GPIO_PORT[Led], LED_GPIO_PIN_SOURCE[Led], LED_GPIO_AF_TIM[Led]);
    }
}

void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b)
{
    TIM2->CCR2 = r;
    TIM2->CCR3 = g;
    TIM2->CCR4 = b;
}

void Get_RGB_LED_Values(uint16_t* values)
{
    values[0] = TIM2->CCR2;
    values[1] = TIM2->CCR3;
    values[2] = TIM2->CCR4;
}

void Set_User_LED(uint8_t state)
{
    if (state)
        LED_GPIO_PORT[LED_USER]->BSRRL = LED_GPIO_PIN[LED_USER];
    else
        LED_GPIO_PORT[LED_USER]->BSRRH = LED_GPIO_PIN[LED_USER];
}

void Toggle_User_LED()
{
    LED_GPIO_PORT[LED_USER]->ODR ^= LED_GPIO_PIN[LED_USER];
}

uint16_t Get_RGB_LED_Max_Value()
{
    return (TIM2->ARR + 1);
}

/**
 * @brief  Configures Button GPIO, EXTI Line and DEBOUNCE Timer.
 * @param  Button: Specifies the Button to be configured.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @param  Button_Mode: Specifies Button mode.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
 *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
 *                     generation capability
 * @retval None
 */
void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the BUTTON Clock */
    RCC_AHB1PeriphClockCmd(BUTTON_GPIO_CLK[Button], ENABLE);

    /* Configure Button pin */
    GPIO_InitStructure.GPIO_Pin = BUTTON_GPIO_PIN[Button];
    GPIO_InitStructure.GPIO_Mode = BUTTON_GPIO_MODE[Button];
    GPIO_InitStructure.GPIO_PuPd = BUTTON_GPIO_PUPD[Button];
    GPIO_Init(BUTTON_GPIO_PORT[Button], &GPIO_InitStructure);

    if (Button_Mode == BUTTON_MODE_EXTI)
    {
        /* Disable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);

        /* Enable the TIM2 Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM2_IRQ_PRIORITY;	//OLD: 0x02
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;							//OLD: 0x00
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

        NVIC_Init(&NVIC_InitStructure);

        /* Enable the Button EXTI Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = BUTTON_IRQn[Button];
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI2_IRQ_PRIORITY;		//OLD: 0x02
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;							//OLD: 0x01
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

        NVIC_Init(&NVIC_InitStructure);

        BUTTON_EXTI_Config(Button, ENABLE);
    }
}

void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* Connect Button EXTI Line to Button GPIO Pin */
    SYSCFG_EXTILineConfig(BUTTON_EXTI_PORT_SOURCE[Button], BUTTON_EXTI_PIN_SOURCE[Button]);

    /* Clear the EXTI line pending flag */
    EXTI_ClearFlag(BUTTON_EXTI_LINE[Button]);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = BUTTON_EXTI_LINE[Button];
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = BUTTON_EXTI_TRIGGER[Button];
    EXTI_InitStructure.EXTI_LineCmd = NewState;
    EXTI_Init(&EXTI_InitStructure);
}

/**
 * @brief  Returns the selected Button non-filtered state.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Actual Button Pressed state.
 */
uint8_t BUTTON_GetState(Button_TypeDef Button)
{
    return GPIO_ReadInputDataBit(BUTTON_GPIO_PORT[Button], BUTTON_GPIO_PIN[Button]);
}

/**
 * @brief  Returns the selected Button Debounced Time.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Button Debounced time in millisec.
 */
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button)
{
    return BUTTON_DEBOUNCED_TIME[Button];
}

void BUTTON_ResetDebouncedState(Button_TypeDef Button)
{
    BUTTON_DEBOUNCED_TIME[Button] = 0;
}

/*******************************************************************************
 * Function Name  : USB_Disconnect_Config
 * Description    : Disconnect pin configuration
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_Disconnect_Config(void)
{
//    GPIO_InitTypeDef GPIO_InitStructure;
//
//    /* Enable USB_DISCONNECT GPIO clock */
//    RCC_APB2PeriphClockCmd(USB_DISCONNECT_GPIO_CLK, ENABLE);
//
//    /* USB_DISCONNECT_GPIO_PIN used as USB pull-up */
//    GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_GPIO_PIN;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
//    GPIO_Init(USB_DISCONNECT_GPIO_PORT, &GPIO_InitStructure);
}

/*******************************************************************************
 * Function Name  : Set_USBClock
 * Description    : Configures USB Clock input (48MHz)
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Set_USBClock(void)
{
//    /* Select USBCLK source */
//    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
//
//    /* Enable the USB clock */
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/*******************************************************************************
 * Function Name  : Enter_LowPowerMode
 * Description    : Power-off system clocks and power while entering suspend mode
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Enter_LowPowerMode(void)
{
//    /* Set the device state to suspend */
//    bDeviceState = SUSPENDED;
}

/*******************************************************************************
 * Function Name  : Leave_LowPowerMode
 * Description    : Restores system clocks and power while exiting suspend mode
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Leave_LowPowerMode(void)
{
//    DEVICE_INFO *pInfo = &Device_Info;
//
//    /* Set the device state to the correct state */
//    if (pInfo->Current_Configuration != 0)
//    {
//        /* Device configured */
//        bDeviceState = CONFIGURED;
//    }
//    else
//    {
//        bDeviceState = ATTACHED;
//    }
}

/*******************************************************************************
 * Function Name  : USB_Interrupts_Config
 * Description    : Configures the USB interrupts
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_Interrupts_Config(void)
{
//    NVIC_InitTypeDef NVIC_InitStructure;
//
//    /* Enable the USB interrupt */
//    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_LP_IRQ_PRIORITY;			//OLD: 0x01
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;								//OLD: 0x00
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
 * Function Name  : USB_Cable_Config
 * Description    : Software Connection/Disconnection of USB Cable
 * Input          : None.
 * Return         : Status
 *******************************************************************************/
void USB_Cable_Config (FunctionalState NewState)
{
//    if (NewState != DISABLE)
//    {
//        GPIO_ResetBits(USB_DISCONNECT_GPIO_PORT, USB_DISCONNECT_GPIO_PIN);
//    }
//    else
//    {
//        GPIO_SetBits(USB_DISCONNECT_GPIO_PORT, USB_DISCONNECT_GPIO_PIN);
//    }
}

void Load_SystemFlags(void)
{
    //To Do
}

void Save_SystemFlags(void)
{
    //To Do
}

void FLASH_WriteProtection_Enable(uint32_t FLASH_Pages)
{
    //To Do
}

void FLASH_WriteProtection_Disable(uint32_t FLASH_Pages)
{
    //To Do
}

void FLASH_Erase(void)
{
    //To Do
}

void FLASH_Backup(uint32_t FLASH_Address)
{
    //To Do
}

void FLASH_Restore(uint32_t FLASH_Address)
{
    //To Do
}

void FLASH_Begin(uint32_t FLASH_Address, uint32_t fileSize)
{
    //To Do
}

uint16_t FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize)
{
    //To Do
    return 0;
}

void FLASH_End(void)
{
    //To Do
}

void FLASH_Read_ServerAddress_Data(void *buf)
{
    //To Do
}

// keyBuffer length must be at least EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH
void FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    //To Do
}

// keyBuffer length must be at least EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH
void FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{
    //To Do
}

void FACTORY_Flash_Reset(void)
{
    //To Do
}

void BACKUP_Flash_Reset(void)
{
    //To Do
}

void OTA_Flash_Reset(void)
{
    //To Do
}

bool OTA_Flashed_GetStatus(void)
{
    //To Do
    return false;
}

void OTA_Flashed_ResetStatus(void)
{
    //To Do
}

/*******************************************************************************
 * Function Name  : Finish_Update.
 * Description    : Reset the device.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Finish_Update(void)
{
    //To Do
}

static volatile system_tick_t system_1ms_tick = 0;

void System1MsTick(void)
{
    system_1ms_tick++;
}

system_tick_t GetSystem1MsTick()
{
    return system_1ms_tick;
}
