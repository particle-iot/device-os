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
#include "dct.h"
#include "usb_dcd.h"

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
const uint16_t BUTTON_EXTI_IRQn[] = {BUTTON1_EXTI_IRQn};
const uint8_t BUTTON_EXTI_IRQ_PRIORITY[] = {BUTTON1_EXTI_IRQ_PRIORITY};
EXTITrigger_TypeDef BUTTON_EXTI_TRIGGER[] = {BUTTON1_EXTI_TRIGGER};

uint32_t Internal_Flash_Address = 0;
uint32_t Internal_Flash_Data = 0;
uint16_t Flash_Update_Index = 0;
volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
#ifdef USE_SERIAL_FLASH
uint32_t External_Flash_Address = 0;
uint8_t External_Flash_Data[4];
static uint32_t External_Flash_Start_Address = 0;
#endif

/* Extern variables ----------------------------------------------------------*/
extern USB_OTG_CORE_HANDLE USB_OTG_dev;

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

    /* Internal Flash Erase/Program fails if this is not called */
    FLASH_ClearFlags();
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

void SysTick_Disable() {
    SysTick->CTRL = SysTick->CTRL & ~SysTick_CTRL_ENABLE_Msk;
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

void IWDG_Reset_Enable(uint32_t msTimeout)
{
    if(SYSTEM_FLAG(IWDG_Enable_SysFlag) == 0xD001)
    {
        if (msTimeout == 0)
        {
            system_flags.IWDG_Enable_SysFlag = 0xFFFF;
            Save_SystemFlags();

            NVIC_SystemReset();
        }

        /* Enable write access to IWDG_PR and IWDG_RLR registers */
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

        /* IWDG counter clock: LSI/256 */
        IWDG_SetPrescaler(IWDG_Prescaler_256);

        /* IWDG timeout may vary due to LSI frequency dispersion */
        msTimeout = ((msTimeout * 32) / 256); //Assuming LSI Frequency = 32000
        if (msTimeout > 0xfff) msTimeout = 0xfff;   // 26214.4

        IWDG_SetReload((uint16_t)msTimeout);

        /* Reload IWDG counter */
        IWDG_ReloadCounter();

        /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
        IWDG_Enable();
    }
}

void UI_Timer_Configure(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* Enable TIM2 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /*
    Since APB1 prescaler is different from 1.
    TIM2CLK = 2 * PCLK1
    PCLK1 = HCLK / 4
    TIM2CLK = HCLK / 2 = SystemCoreClock / 2
    */
    /* TIM2 Update Frequency = 120000000/2/60/10000 = 100Hz = 10ms */
    /* TIM2_Prescaler: 59 */
    /* TIM2_Autoreload: 9999 -> 100Hz = 10ms */
    uint16_t TIM2_Prescaler = (uint16_t)((SystemCoreClock / 2) / 1000000) - 1;
    uint16_t TIM2_Autoreload = (uint16_t)(1000000 / UI_TIMER_FREQUENCY) - 1;

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    /* Time Base Configuration */
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
#ifdef RGB_LINES_REVERSED
    TIM2->CCR4 = r;
    TIM2->CCR3 = g;
    TIM2->CCR2 = b;
#else
    TIM2->CCR2 = r;
    TIM2->CCR3 = g;
    TIM2->CCR4 = b;
#endif
}

void Get_RGB_LED_Values(uint16_t* values)
{
#ifdef RGB_LINES_REVERSED
    values[0] = TIM2->CCR2;
    values[1] = TIM2->CCR3;
    values[2] = TIM2->CCR4;
#else
    values[0] = TIM2->CCR4;
    values[1] = TIM2->CCR3;
    values[2] = TIM2->CCR2;
#endif
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
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM2_IRQ_PRIORITY;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

        NVIC_Init(&NVIC_InitStructure);

        /* Enable the Button EXTI Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = BUTTON_EXTI_IRQn[Button];
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BUTTON_EXTI_IRQ_PRIORITY[Button];
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
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

#ifdef USE_SERIAL_FLASH
/**
 * @brief  DeInitializes the peripherals used by the SPI FLASH driver.
 * @param  None
 * @retval None
 */
void sFLASH_SPI_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Disable the sFLASH_SPI  */
    SPI_Cmd(sFLASH_SPI, DISABLE);

    /* DeInitializes the sFLASH_SPI */
    SPI_I2S_DeInit(sFLASH_SPI);

    /* sFLASH_SPI Peripheral clock disable */
    sFLASH_SPI_CLK_CMD(sFLASH_SPI_CLK, DISABLE);

    /* Configure all pins used by the SPI as input floating */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* Configure sFLASH_SPI pin: SCK */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_SCK_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pin: MISO */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MISO_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pin: MOSI */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MOSI_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI_CS_GPIO_PIN pin: sFLASH CS pin */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_CS_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_CS_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief  Initializes the peripherals used by the SPI FLASH driver.
 * @param  None
 * @retval None
 */
void sFLASH_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* sFLASH_SPI_CS_GPIO, sFLASH_SPI_SCK_GPIOs, FLASH_SPI_MOSI_GPIO
       and sFLASH_SPI_MISO_GPIO Periph clock enable */
    RCC_AHB1PeriphClockCmd(sFLASH_SPI_CS_GPIO_CLK | sFLASH_SPI_SCK_GPIO_CLK |
            sFLASH_SPI_MOSI_GPIO_CLK | sFLASH_SPI_MISO_GPIO_CLK, ENABLE);

    /* sFLASH_SPI Periph clock enable */
    sFLASH_SPI_CLK_CMD(sFLASH_SPI_CLK, ENABLE);

    /* Connect SPI pins to AF5 */
    GPIO_PinAFConfig(sFLASH_SPI_SCK_GPIO_PORT, sFLASH_SPI_SCK_SOURCE, sFLASH_SPI_SCK_AF);
    GPIO_PinAFConfig(sFLASH_SPI_MISO_GPIO_PORT, sFLASH_SPI_MISO_SOURCE, sFLASH_SPI_MISO_AF);
    GPIO_PinAFConfig(sFLASH_SPI_MOSI_GPIO_PORT, sFLASH_SPI_MOSI_SOURCE, sFLASH_SPI_MOSI_AF);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    /* Configure sFLASH_SPI pin: SCK */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_SCK_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pin: MOSI */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MOSI_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pin: MISO */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MISO_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI_CS_GPIO_PIN pin: sFLASH CS pin */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_CS_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(sFLASH_SPI_CS_GPIO_PORT, &GPIO_InitStructure);

    /* Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();

    /* SPI configuration */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = sFLASH_SPI_BAUDRATE_PRESCALER;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(sFLASH_SPI, &SPI_InitStructure);

    /* Enable the sFLASH_SPI  */
    SPI_Cmd(sFLASH_SPI, ENABLE);
}

/* Select sFLASH: Chip Select pin low */
void sFLASH_CS_LOW(void)
{
    GPIO_ResetBits(sFLASH_SPI_CS_GPIO_PORT, sFLASH_SPI_CS_GPIO_PIN);
}

/* Deselect sFLASH: Chip Select pin high */
void sFLASH_CS_HIGH(void)
{
    GPIO_SetBits(sFLASH_SPI_CS_GPIO_PORT, sFLASH_SPI_CS_GPIO_PIN);
}
#endif

/*******************************************************************************
 * Function Name  : USB_Cable_Config
 * Description    : Software Connection/Disconnection of USB Cable
 * Input          : None.
 * Return         : Status
 *******************************************************************************/
void USB_Cable_Config (FunctionalState NewState)
{
    if (NewState != DISABLE)
    {
        /* Connect device (enable internal pull-up) */
        DCD_DevConnect(&USB_OTG_dev);
    }
    else
    {
        /* Disconnect device (disable internal pull-up) */
        DCD_DevDisconnect(&USB_OTG_dev);
    }
}

inline void Load_SystemFlags_Impl(platform_system_flags_t* flags)
{
    const void* flags_store = dct_read_app_data(0);
    memcpy(flags, flags_store, sizeof(platform_system_flags_t));    
    flags->header[0] = 0xACC0;
    flags->header[1] = 0x1ADE;
}

inline void Save_SystemFlags_Impl(const platform_system_flags_t* flags)
{
    dct_write_app_data(flags, 0, sizeof(*flags));        
}

platform_system_flags_t system_flags;

void Load_SystemFlags() 
{
    Load_SystemFlags_Impl(&system_flags);
}

void Save_SystemFlags() 
{
    Save_SystemFlags_Impl(&system_flags);
}

uint16_t FLASH_SectorToErase(FlashDevice_TypeDef flashDeviceID, uint32_t startAddress)
{
    uint16_t flashSector = 0xFFFF;//Invalid sector

    if (flashDeviceID != FLASH_INTERNAL)
    {
        return flashSector;
    }

    if (startAddress < 0x08004000)
    {
        flashSector = FLASH_Sector_0;
    }
    else if (startAddress < 0x08008000)
    {
        flashSector = FLASH_Sector_1;
    }
    else if (startAddress < 0x0800C000)
    {
        flashSector = FLASH_Sector_2;
    }
    else if (startAddress < 0x08010000)
    {
        flashSector = FLASH_Sector_3;
    }
    else if (startAddress < 0x08020000)
    {
        flashSector = FLASH_Sector_4;
    }
    else if (startAddress < 0x08040000)
    {
        flashSector = FLASH_Sector_5;
    }
    else if (startAddress < 0x08060000)
    {
        flashSector = FLASH_Sector_6;
    }
    else if (startAddress < 0x08080000)
    {
        flashSector = FLASH_Sector_7;
    }
    else if (startAddress < 0x080A0000)
    {
        flashSector = FLASH_Sector_8;
    }
    else if (startAddress < 0x080C0000)
    {
        flashSector = FLASH_Sector_9;
    }
    else if (startAddress < 0x080E0000)
    {
        flashSector = FLASH_Sector_10;
    }
    else if (startAddress < 0x08100000)
    {
        flashSector = FLASH_Sector_11;
    }

    return flashSector;
}

bool FLASH_CheckValidAddressRange(FlashDevice_TypeDef flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_INTERNAL)
    {
        if (startAddress < 0x08020000 || endAddress >= 0x08100000)
        {
            return false;
        }
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        if (startAddress < 0x4000 || endAddress >= 0x100000)
        {
            return false;
        }
#else
        return false;
#endif
    }
    else
    {
        return false;   //Invalid FLASH ID
    }

    return true;
}

bool FLASH_EraseMemory(FlashDevice_TypeDef flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t eraseCounter = 0;
    uint32_t numPages = 0;

    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        /* Check which sector has to be erased */
        uint16_t flashSector = FLASH_SectorToErase(FLASH_INTERNAL, startAddress);

        if (flashSector > FLASH_Sector_11)
        {
            return false;
        }

        /* Unlock the Flash Program Erase Controller */
        FLASH_Unlock();

        /* Define the number of Internal Flash pages to be erased */
        numPages = FLASH_PagesMask(length, INTERNAL_FLASH_PAGE_SIZE);

        /* Clear All pending flags */
        FLASH_ClearFlags();

        /* Erase the Internal Flash pages */
        for (eraseCounter = 0; (eraseCounter < numPages); eraseCounter++)
        {
            FLASHStatus = FLASH_EraseSector(flashSector + (8 * eraseCounter), VoltageRange_3);

            /* If erase operation fails, return Failure */
            if (FLASHStatus != FLASH_COMPLETE)
            {
                return false;
            }
        }

        /* Locks the FLASH Program Erase Controller */
        FLASH_Lock();

        return true;
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();

        /* Define the number of External Flash pages to be erased */
        numPages = FLASH_PagesMask(length, sFLASH_PAGESIZE);

        /* Erase the SPI Flash pages */
        for (eraseCounter = 0; (eraseCounter < numPages); eraseCounter++)
        {
            sFLASH_EraseSector(startAddress + (sFLASH_PAGESIZE * eraseCounter));
        }

        /* Return Success */
        return true;
#endif
    }

    /* Return Failure */
    return false;
}

bool FLASH_CopyMemory(uint8_t sourceDeviceID, uint32_t sourceAddress,
                      uint8_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length)
{
#ifdef USE_SERIAL_FLASH
    uint8_t serialFlashData[4];
#endif
    uint32_t internalFlashData = 0;
    uint32_t endAddress = sourceAddress + length - 1;

    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    if (sourceDeviceID == FLASH_SERIAL || destinationDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    if (FLASH_EraseMemory(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    if (sourceDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    if (destinationDeviceID == FLASH_INTERNAL)
    {
        /* Unlock the internal flash program erase controller */
        FLASH_Unlock();
    }

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        if (sourceDeviceID == FLASH_INTERNAL)
        {
            /* Read data from internal flash source address */
            internalFlashData = (*(__IO uint32_t*) sourceAddress);
        }
#ifdef USE_SERIAL_FLASH
        else if (sourceDeviceID == FLASH_SERIAL)
        {
            /* Read data from serial flash source address */
            sFLASH_ReadBuffer(serialFlashData, sourceAddress, 4);
        }
#endif

        if (destinationDeviceID == FLASH_INTERNAL)
        {
#ifdef USE_SERIAL_FLASH
            if (sourceDeviceID == FLASH_SERIAL)
            {
                internalFlashData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
            }
#endif

            /* Program data to internal flash destination address */
            FLASHStatus = FLASH_ProgramWord(destinationAddress, internalFlashData);

            /* If program operation fails, return Failure */
            if (FLASHStatus != FLASH_COMPLETE)
            {
                return false;
            }
        }
#ifdef USE_SERIAL_FLASH
        else if (destinationDeviceID == FLASH_SERIAL)
        {
            if (sourceDeviceID == FLASH_INTERNAL)
            {
                serialFlashData[0] = (uint8_t)(internalFlashData & 0xFF);
                serialFlashData[1] = (uint8_t)((internalFlashData & 0xFF00) >> 8);
                serialFlashData[2] = (uint8_t)((internalFlashData & 0xFF0000) >> 16);
                serialFlashData[3] = (uint8_t)((internalFlashData & 0xFF000000) >> 24);
            }

            /* Program data to serial flash destination address */
            sFLASH_WriteBuffer(serialFlashData, destinationAddress, 4);
        }
#endif

        sourceAddress += 4;
        destinationAddress += 4;
    }

    if (destinationDeviceID == FLASH_INTERNAL)
    {
        /* Locks the internal flash program erase controller */
        FLASH_Lock();
    }

    return true;
}

//This function called in bootloader to perform the memory update process
void FLASH_Update_Modules(void)
{
    platform_flash_modules_t flash_modules[FLASH_MODULES_MAX];
    uint8_t flash_module_index = 0;
    uint8_t updateStatusFlags = 0x0;

    //Fill the Flash modules info data read from the dct area
    const void* dct_app_data = dct_read_app_data(DCT_FLASH_MODULES_OFFSET);
    memcpy(flash_modules, dct_app_data, sizeof(flash_modules));

    for (flash_module_index = 0; flash_module_index < FLASH_MODULES_MAX; flash_module_index++)
    {
        if(flash_modules[flash_module_index].statusFlag == 0x1)
        {
            //Copy memory from source to destination based on flash device id
            bool copy_result = FLASH_CopyMemory(flash_modules[flash_module_index].sourceDeviceID,
                                                flash_modules[flash_module_index].sourceAddress,
                                                flash_modules[flash_module_index].destinationDeviceID,
                                                flash_modules[flash_module_index].destinationAddress,
                                                flash_modules[flash_module_index].length);

            if(copy_result != false)
            {
                updateStatusFlags |= flash_modules[flash_module_index].statusFlag;
                flash_modules[flash_module_index].statusFlag = 0x0; //Reset statusFlag
            }
        }
    }

    if(updateStatusFlags == 0x1)
    {
        //Only update DCT if required
        dct_write_app_data(flash_modules, DCT_FLASH_MODULES_OFFSET, sizeof(flash_modules));
    }
}

void FLASH_ClearFlags(void)
{
    /* Clear All pending flags */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors)
{
    /* Get FLASH_Sectors write protection status */
    uint32_t SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

    if (SectorsWRPStatus != 0)
    {
        //If FLASH_Sectors are not write protected, enable the write protection

        /* Enable the Flash option control register access */
        FLASH_OB_Unlock();

        /* Enable FLASH_Sectors write protection */
        FLASH_OB_WRPConfig(FLASH_Sectors, ENABLE);

        /* Start the Option Bytes programming process */
        if (FLASH_OB_Launch() != FLASH_COMPLETE)
        {
            //Error during Option Bytes programming process
        }

        /* Disable the Flash option control register access */
        FLASH_OB_Lock();

        /* Get FLASH_Sectors write protection status */
        SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

        /* Check if FLASH_Sectors are write protected */
        if (SectorsWRPStatus == 0)
        {
            //Write Protection Enable Operation is done correctly
        }

        /* Generate System Reset (not mandatory on F2 series ???) */
        NVIC_SystemReset();
    }
}

void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors)
{
    /* Get FLASH_Sectors write protection status */
    uint32_t SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

    if (SectorsWRPStatus == 0)
    {
        //If FLASH_Sectors are write protected, disable the write protection

        /* Enable the Flash option control register access */
        FLASH_OB_Unlock();

        /* Disable FLASH_Sectors write protection */
        FLASH_OB_WRPConfig(FLASH_Sectors, DISABLE);

        /* Start the Option Bytes programming process */
        if (FLASH_OB_Launch() != FLASH_COMPLETE)
        {
            //Error during Option Bytes programming process
        }

        /* Disable the Flash option control register access */
        FLASH_OB_Lock();

        /* Get FLASH_Sectors write protection status */
        SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

        /* Check if FLASH_Sectors write protection is disabled */
        if (SectorsWRPStatus == FLASH_Sectors)
        {
            //Write Protection Disable Operation is done correctly
        }

        /* Generate System Reset (not mandatory on F2 series ???) */
        NVIC_SystemReset();
    }
}

void FLASH_Erase(void)
{
    FLASH_EraseMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
}

void FLASH_Backup(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    FLASH_CopyMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FLASH_SERIAL, FLASH_Address, FIRMWARE_IMAGE_SIZE);
#endif
}

void FLASH_Restore(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
#endif
}

uint32_t FLASH_PagesMask(uint32_t imageSize, uint32_t pageSize)
{
    //Calculate the number of flash pages that needs to be erased
    uint32_t numPages = 0x0;

    if ((imageSize % pageSize) != 0)
    {
        numPages = (imageSize / pageSize) + 1;
    }
    else
    {
        numPages = imageSize / pageSize;
    }

    return numPages;
}

void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize)
{
#ifdef USE_SERIAL_FLASH
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();

    Flash_Update_Index = 0;
    External_Flash_Start_Address = FLASH_Address;
    External_Flash_Address = External_Flash_Start_Address;

    FLASH_EraseMemory(FLASH_SERIAL, External_Flash_Start_Address, imageSize);
#endif
}

uint16_t FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize)
{
#ifdef USE_SERIAL_FLASH
    uint8_t *writeBuffer = pBuffer;
    uint8_t readBuffer[bufferSize];

    /* Write Data Buffer to SPI Flash memory */
    sFLASH_WriteBuffer(writeBuffer, External_Flash_Address, bufferSize);

    /* Read Data Buffer from SPI Flash memory */
    sFLASH_ReadBuffer(readBuffer, External_Flash_Address, bufferSize);

    /* Is the Data Buffer successfully programmed to SPI Flash memory */
    if (0 == memcmp(writeBuffer, readBuffer, bufferSize))
    {
        External_Flash_Address += bufferSize;
        Flash_Update_Index += 1;
    }
    else
    {
        /* Erase the problematic SPI Flash pages and back off the chunk index */
        External_Flash_Address = ((uint32_t)(External_Flash_Address / sFLASH_PAGESIZE)) * sFLASH_PAGESIZE;
        sFLASH_EraseSector(External_Flash_Address);
        Flash_Update_Index = (uint16_t)((External_Flash_Address - External_Flash_Start_Address) / bufferSize);
    }
#endif

    return Flash_Update_Index;
}

void FLASH_End(void)
{
#ifdef USE_SERIAL_FLASH
    system_flags.FLASH_OTA_Update_SysFlag = 0x0005;
    Save_SystemFlags();

    RTC_WriteBackupRegister(RTC_BKP_DR10, 0x0005);

    USB_Cable_Config(DISABLE);

    NVIC_SystemReset();
#endif
}

void FACTORY_Flash_Reset(void)
{
#ifdef USE_SERIAL_FLASH
    // Restore the Factory programmed application firmware from External Flash
    FLASH_Restore(EXTERNAL_FLASH_FAC_ADDRESS);

    system_flags.Factory_Reset_SysFlag = 0xFFFF;

    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;

    Finish_Update();
#endif
}

void BACKUP_Flash_Reset(void)
{
#ifdef USE_SERIAL_FLASH
    //Restore the Backup programmed application firmware from External Flash
    FLASH_Restore(EXTERNAL_FLASH_BKP_ADDRESS);

    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;

    Finish_Update();
#endif
}

void OTA_Flash_Reset(void)
{
#ifdef USE_SERIAL_FLASH
/*
    First take backup of the current application firmware to External Flash
    Commented for BM-14 since there's not much external flash space.
    By uncommenting the below code, the crucial factory reset firmware
    is at risk of corruption.

    //FLASH_Backup(EXTERNAL_FLASH_BKP_ADDRESS);
*/

/*
    Following code commented for BM-xx since interrupting the bootloader while it
    is copying from external to internal flash: the copy process should be repeated.
    Reverting to factory firmware is NOT an outcome here.

    //system_flags.FLASH_OTA_Update_SysFlag = 0x5555;
    //Save_SystemFlags();
    //RTC_WriteBackupRegister(RTC_BKP_DR10, 0x5555);
*/

    //Restore the OTA programmed application firmware from External Flash
    FLASH_Restore(EXTERNAL_FLASH_OTA_ADDRESS);

    //Set system flag if OTA firmware update is successful
    //the same should be reset during restore operation
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0001;

    Finish_Update();
#endif
}

bool OTA_Flashed_GetStatus(void)
{
    if(system_flags.OTA_FLASHED_Status_SysFlag == 0x0001)
        return true;
    else
        return false;
}

void OTA_Flashed_ResetStatus(void)
{
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();
}

/*******************************************************************************
 * Function Name  : Finish_Update.
 * Description    : Reset the device.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Finish_Update(void)
{
    //Set system flag to Enable IWDG in IWDG_Reset_Enable()
    //called in bootloader to recover from corrupt firmware
    system_flags.IWDG_Enable_SysFlag = 0xD001;

    system_flags.FLASH_OTA_Update_SysFlag = 0x5000;
    Save_SystemFlags();

    RTC_WriteBackupRegister(RTC_BKP_DR10, 0x5000);

    USB_Cable_Config(DISABLE);

    NVIC_SystemReset();
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
