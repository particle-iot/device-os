/**
 ******************************************************************************
 * @file    hw_config.c
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Hardware Configuration & Setup
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#include "usb_lib.h"
#include "usb_pwr.h"
#include <string.h>
#include "spi_bus.h"
#include "debug.h"
#include "cc3000_spi.h"
#include "rgbled_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#if defined(DEBUG_BUILD)
#define DBGMCU_SETTINGS (DBGMCU_CR_DBG_SLEEP|DBGMCU_CR_DBG_STOP|DBGMCU_CR_DBG_STANDBY|DBGMCU_CR_DBG_IWDG_STOP|DBGMCU_CR_DBG_WWDG_STOP)
#else
#define DBGMCU_SETTINGS (DBGMCU_CR_DBG_IWDG_STOP|DBGMCU_CR_DBG_WWDG_STOP)
#endif

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t USE_SYSTEM_FLAGS = 0;	//0, 1
uint16_t sys_health_cache = 0; // Used by the SYS_HEALTH macros store new heath if higher

GPIO_TypeDef* LED_GPIO_PORT[] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT, LED4_GPIO_PORT};
const uint16_t LED_GPIO_PIN[] = {LED1_GPIO_PIN, LED2_GPIO_PIN, LED3_GPIO_PIN, LED4_GPIO_PIN};
const uint32_t LED_GPIO_CLK[] = {LED1_GPIO_CLK, LED2_GPIO_CLK, LED3_GPIO_CLK, LED4_GPIO_CLK};

GPIO_TypeDef* BUTTON_GPIO_PORT[] = {BUTTON1_GPIO_PORT, BUTTON2_GPIO_PORT};
const uint16_t BUTTON_GPIO_PIN[] = {BUTTON1_GPIO_PIN, BUTTON2_GPIO_PIN};
const uint32_t BUTTON_GPIO_CLK[] = {BUTTON1_GPIO_CLK, BUTTON2_GPIO_CLK};
GPIOMode_TypeDef BUTTON_GPIO_MODE[] = {BUTTON1_GPIO_MODE, BUTTON2_GPIO_MODE};
__IO uint16_t BUTTON_DEBOUNCED_TIME[] = {0, 0};

const uint16_t BUTTON_EXTI_LINE[] = {BUTTON1_EXTI_LINE, BUTTON2_EXTI_LINE};
const uint16_t BUTTON_GPIO_PORT_SOURCE[] = {BUTTON1_EXTI_PORT_SOURCE, BUTTON2_EXTI_PORT_SOURCE};
const uint16_t BUTTON_GPIO_PIN_SOURCE[] = {BUTTON1_EXTI_PIN_SOURCE, BUTTON2_EXTI_PIN_SOURCE};
const uint16_t BUTTON_IRQn[] = {BUTTON1_EXTI_IRQn, BUTTON2_EXTI_IRQn};
EXTITrigger_TypeDef BUTTON_EXTI_TRIGGER[] = {BUTTON1_EXTI_TRIGGER, BUTTON2_EXTI_TRIGGER};

uint16_t Bootloader_Version_SysFlag = 0xFFFF;
uint16_t NVMEM_SPARK_Reset_SysFlag = 0xFFFF;
uint16_t FLASH_OTA_Update_SysFlag = 0xFFFF;
uint16_t OTA_FLASHED_Status_SysFlag = 0xFFFF;
uint16_t Factory_Reset_SysFlag = 0xFFFF;
uint8_t dfu_on_no_firmware = 0xFF;
uint8_t Factory_Reset_Done_SysFlag = 0xFF;
uint8_t StartupMode_SysFlag = 0xFF;
uint8_t FeaturesEnabled_SysFlag = 0xFF;
uint32_t RCC_CSR_SysFlag = !0x0;

uint32_t WRPR_Value = 0xFFFFFFFF;
uint32_t Flash_Pages_Protected = 0x0;

__IO uint16_t CC3000_SPI_CR;
__IO uint16_t sFLASH_SPI_CR;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Initialise Data Watchpoint and Trace Register (DWT).
 * @param  None
 * @retval None
 *
 *
 */

static void DWT_Init(void)
{
    DBGMCU->CR |= DBGMCU_SETTINGS;
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
	 file (startup_stm32f10x_xx.S) before to branch to application main.
	 To reconfigure the default setting of SystemInit() function, refer to
	 system_stm32f10x.c file
     */

    /* Enable PWR and BKP clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    /* Enable write access to Backup domain */
    PWR_BackupAccessCmd(ENABLE);

    /* Should we execute System Standby mode */
    // Use "HAL_Core_Execute_Standby_Mode()" defined in core_hal.c
    // Or Use below code
    if(BKP_ReadBackupRegister(BKP_DR9) == 0xA5A5)
    {
        /* Clear Standby mode system flag */
        BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

        /* Request to enter STANDBY mode */
        PWR_EnterSTANDBYMode();

        /* Following code will not be reached */
        while(1);
    }

    DWT_Init();

    /* NVIC configuration */
    NVIC_Configuration();

    /* Enable AFIO clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* Configure TIM1 for LED-PWM and BUTTON-DEBOUNCE usage */
    UI_Timer_Configure();

    /* Configure the LEDs and set the default states */
    int LEDx;
    for(LEDx = 1; LEDx < LEDn; ++LEDx)
    {
        // LED_USER initialization is skipped during system setup
        LED_Init(LEDx);
    }

    /* Configure the Button */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);

    /* Sometimes Internal Flash Erase/Program fails if this is not called */
    FLASH_ClearFlags();
}

void Reset_System(void)
{
    SysTick_Disable();
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

void IWDG_Reset_Enable(uint32_t msTimeout)
{
    // Do not enable IWDG if Stop Mode Flag is set
    if((BKP_ReadBackupRegister(BKP_DR9) >> 12) != 0xA)
    {
        /* Enable write access to IWDG_PR and IWDG_RLR registers */
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

        /* IWDG counter clock: LSI/256 */
        IWDG_SetPrescaler(IWDG_Prescaler_256);

        /* IWDG timeout may vary due to LSI frequency dispersion */
        msTimeout = ((msTimeout * 40) / 256); //Assuming LSI Frequency = 40000
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

    /* Enable TIM1 clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    /* TIM1 Update Frequency = 72000000/72/10000 = 100Hz = 10ms */
    /* TIM1_Prescaler: 72 */
    /* TIM1_Autoreload: 9999 -> 100Hz = 10ms */
    uint16_t TIM1_Prescaler = SystemCoreClock / 1000000;
    uint16_t TIM1_Autoreload = (1000000 / UI_TIMER_FREQUENCY) - 1;

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    /* Time Base Configuration */
    TIM_TimeBaseStructure.TIM_Period = TIM1_Autoreload;
    TIM_TimeBaseStructure.TIM_Prescaler = TIM1_Prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0000;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_OCStructInit(&TIM_OCInitStructure);

    /* PWM1 Mode configuration: Channel 1, 2 and 3 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0x0000;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Disable);

    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable);

    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Disable);

    /* Output Compare Timing Mode configuration: Channel 4 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0x0000;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC4Init(TIM1, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* TIM1 enable counter */
    TIM_Cmd(TIM1, ENABLE);

    /* Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
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
    RCC_APB2PeriphClockCmd(LED_GPIO_CLK[Led], ENABLE);

    /* Configure the GPIO_LED pin as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN[Led];
    if(Led == LED_USER)
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    else
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(LED_GPIO_PORT[Led], &GPIO_InitStructure);

    Set_User_LED(DISABLE);
}

void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b) {
    TIM1->CCR2 = r;
    TIM1->CCR3 = g;
    TIM1->CCR1 = b;
}

void Get_RGB_LED_Values(uint16_t* values) {
    values[0] = TIM1->CCR2;
    values[1] = TIM1->CCR3;
    values[2] = TIM1->CCR1;
}

void Set_User_LED(uint8_t state) {
    if (state)
        LED_GPIO_PORT[LED_USER]->BSRR = LED_GPIO_PIN[LED_USER];
    else
        LED_GPIO_PORT[LED_USER]->BRR = LED_GPIO_PIN[LED_USER];
}

void Toggle_User_LED() {
    LED_GPIO_PORT[LED_USER]->ODR ^= LED_GPIO_PIN[LED_USER];
}

uint16_t Get_RGB_LED_Max_Value() {
    return (TIM1->ARR + 1);
}

/**
 * @brief  Configures Button GPIO, EXTI Line and DEBOUNCE Timer.
 * @param  Button: Specifies the Button to be configured.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 *     @arg BUTTON2: Button2
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
    RCC_APB2PeriphClockCmd(BUTTON_GPIO_CLK[Button] | RCC_APB2Periph_AFIO, ENABLE);

    /* Configure Button pin as input floating */
    GPIO_InitStructure.GPIO_Mode = BUTTON_GPIO_MODE[Button];
    GPIO_InitStructure.GPIO_Pin = BUTTON_GPIO_PIN[Button];
    GPIO_Init(BUTTON_GPIO_PORT[Button], &GPIO_InitStructure);

    if (Button_Mode == BUTTON_MODE_EXTI)
    {
        /* Disable TIM1 CC4 Interrupt */
        TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);

        /* Enable the TIM1 Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM1_CC_IRQ_PRIORITY;	//OLD: 0x02
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
    GPIO_EXTILineConfig(BUTTON_GPIO_PORT_SOURCE[Button], BUTTON_GPIO_PIN_SOURCE[Button]);

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
 *     @arg BUTTON2: Button2
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
 *     @arg BUTTON2: Button2
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

/**
 * @brief  Initialize the CC3000 - CS and ENABLE lines.
 * @param  None
 * @retval None
 */
void CC3000_WIFI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* CC3000_WIFI_CS_GPIO and CC3000_WIFI_EN_GPIO Peripheral clock enable */
    RCC_APB2PeriphClockCmd(CC3000_WIFI_CS_GPIO_CLK | CC3000_WIFI_EN_GPIO_CLK, ENABLE);

    /* Configure CC3000_WIFI pins: CS */
    GPIO_InitStructure.GPIO_Pin = CC3000_WIFI_CS_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(CC3000_WIFI_CS_GPIO_PORT, &GPIO_InitStructure);

    /* Deselect CC3000 */
    CC3000_CS_HIGH();

    /* Configure CC3000_WIFI pins: Enable */
    GPIO_InitStructure.GPIO_Pin = CC3000_WIFI_EN_GPIO_PIN;
    GPIO_Init(CC3000_WIFI_EN_GPIO_PORT, &GPIO_InitStructure);

    /* Disable CC3000 */
    CC3000_Write_Enable_Pin(WLAN_DISABLE);
}

/**
 * @brief  Initialize and configure the SPI peripheral used by CC3000.
 * @param  None
 * @retval None
 */
void CC3000_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    /* CC3000_SPI_SCK_GPIO, CC3000_SPI_MOSI_GPIO and CC3000_SPI_MISO_GPIO Peripheral clock enable */
    RCC_APB2PeriphClockCmd(CC3000_SPI_SCK_GPIO_CLK | CC3000_SPI_MOSI_GPIO_CLK | CC3000_SPI_MISO_GPIO_CLK, ENABLE);

    /* CC3000_SPI Peripheral clock enable */
    CC3000_SPI_CLK_CMD(CC3000_SPI_CLK, ENABLE);

    /* Configure CC3000_SPI pins: SCK */
    GPIO_InitStructure.GPIO_Pin = CC3000_SPI_SCK_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(CC3000_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* Configure CC3000_SPI pins: MOSI */
    GPIO_InitStructure.GPIO_Pin = CC3000_SPI_MOSI_GPIO_PIN;
    GPIO_Init(CC3000_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* Configure CC3000_SPI pins: MISO */
    GPIO_InitStructure.GPIO_Pin = CC3000_SPI_MISO_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(CC3000_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

    /* CC3000_SPI Config */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = CC3000_SPI_BAUDRATE_PRESCALER;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(CC3000_SPI, &SPI_InitStructure);

    CC3000_SPI_CR = CC3000_SPI->CR1;
}

/**
 * @brief  Configure the DMA Peripheral used to handle CC3000 communication via SPI.
 * @param  None
 * @retval None
 */
void CC3000_DMA_Config(CC3000_DMADirection_TypeDef Direction, uint8_t* buffer, uint16_t NumData)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(CC3000_SPI_DMA_CLK, ENABLE);

    DMA_InitStructure.DMA_PeripheralBaseAddr = CC3000_SPI_DR_BASE;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) buffer;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    /* DMA used for Reception */
    if (Direction == CC3000_DMA_RX)
    {
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
        DMA_InitStructure.DMA_BufferSize = NumData;
        DMA_DeInit(CC3000_SPI_RX_DMA_CHANNEL );
        DMA_Init(CC3000_SPI_RX_DMA_CHANNEL, &DMA_InitStructure);
    }
    /* DMA used for Transmission */
    else if (Direction == CC3000_DMA_TX)
    {
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
        DMA_InitStructure.DMA_BufferSize = NumData;
        DMA_DeInit(CC3000_SPI_TX_DMA_CHANNEL );
        DMA_Init(CC3000_SPI_TX_DMA_CHANNEL, &DMA_InitStructure);
    }
}

void CC3000_SPI_DMA_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure and enable SPI DMA TX Channel interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = CC3000_SPI_TX_DMA_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = DMA1_CHANNEL5_IRQ_PRIORITY;	//OLD: 0x00
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;								//OLD: 0x00
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    CC3000_SPI_Init();

    /* Configure DMA Peripheral but don't send data*/
    CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) wlan_rx_buffer, 0);
    CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) wlan_tx_buffer, 0);

    /* Enable SPI DMA TX Channel Transfer Complete Interrupt */
    DMA_ITConfig(CC3000_SPI_TX_DMA_CHANNEL, DMA_IT_TC, ENABLE);

    /* Enable SPI DMA request */
    SPI_I2S_DMACmd(CC3000_SPI, SPI_I2S_DMAReq_Rx, ENABLE);
    SPI_I2S_DMACmd(CC3000_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

    /* Enable CC3000_SPI */
    SPI_Cmd(CC3000_SPI, ENABLE);

    /* Enable DMA RX Channel */
    DMA_Cmd(CC3000_SPI_RX_DMA_CHANNEL, ENABLE);
    /* Enable DMA TX Channel */
    DMA_Cmd(CC3000_SPI_TX_DMA_CHANNEL, ENABLE);
}

void CC3000_SPI_DMA_Channels(FunctionalState NewState)
{
    /* Enable/Disable DMA RX Channel */
    DMA_Cmd(CC3000_SPI_RX_DMA_CHANNEL, NewState);
    /* Enable/Disable DMA TX Channel */
    DMA_Cmd(CC3000_SPI_TX_DMA_CHANNEL, NewState);
}

/* Select CC3000: ChipSelect pin low */
void CC3000_CS_LOW(void)
{
    acquire_spi_bus(BUS_OWNER_CC3000);
    CC3000_SPI->CR1 &= ((uint16_t)0xFFBF);
    CC3000_SPI->CR1 = CC3000_SPI_CR | ((uint16_t)0x0040);
    GPIO_ResetBits(CC3000_WIFI_CS_GPIO_PORT, CC3000_WIFI_CS_GPIO_PIN);
}

/* Deselect CC3000: ChipSelect pin high */
void CC3000_CS_HIGH(void)
{
    GPIO_SetBits(CC3000_WIFI_CS_GPIO_PORT, CC3000_WIFI_CS_GPIO_PIN);
    release_spi_bus(BUS_OWNER_CC3000);
}

/* CC3000 Hardware related callbacks passed to wlan_init */
long CC3000_Read_Interrupt_Pin(void)
{
    return GPIO_ReadInputDataBit(CC3000_WIFI_INT_GPIO_PORT, CC3000_WIFI_INT_GPIO_PIN );
}

void CC3000_Interrupt_Enable(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* CC3000_WIFI_INT_GPIO and AFIO Peripheral clock enable */
    RCC_APB2PeriphClockCmd(CC3000_WIFI_INT_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

    /* Configure CC3000_WIFI pins: Interrupt */
    GPIO_InitStructure.GPIO_Pin = CC3000_WIFI_INT_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(CC3000_WIFI_INT_GPIO_PORT, &GPIO_InitStructure);

    /* Select the CC3000_WIFI_INT GPIO pin used as EXTI Line */
    GPIO_EXTILineConfig(CC3000_WIFI_INT_EXTI_PORT_SOURCE, CC3000_WIFI_INT_EXTI_PIN_SOURCE );

    /* Clear the EXTI line pending flag */
    EXTI_ClearFlag(CC3000_WIFI_INT_EXTI_LINE );

    /* Configure and Enable CC3000_WIFI_INT EXTI line */
    EXTI_InitStructure.EXTI_Line = CC3000_WIFI_INT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set CC3000_WIFI_INT EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = CC3000_WIFI_INT_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI15_10_IRQ_PRIORITY;		//OLD: 0x00
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;								//OLD: 0x01
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void CC3000_Interrupt_Disable(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Disable CC3000_WIFI_INT EXTI Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = CC3000_WIFI_INT_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Disable CC3000_WIFI_INT EXTI line */
    EXTI_InitStructure.EXTI_Line = CC3000_WIFI_INT_EXTI_LINE;
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void CC3000_Write_Enable_Pin(unsigned char val)
{
    /* Set WLAN Enable/Disable */
    if (val != WLAN_DISABLE)
    {
        GPIO_SetBits(CC3000_WIFI_EN_GPIO_PORT, CC3000_WIFI_EN_GPIO_PIN );
    }
    else
    {
        GPIO_ResetBits(CC3000_WIFI_EN_GPIO_PORT, CC3000_WIFI_EN_GPIO_PIN );
    }
}

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

    /* Configure sFLASH_SPI pins: SCK */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_SCK_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(sFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pins: MISO */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MISO_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pins: MOSI */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MOSI_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_MEM_CS_GPIO_PIN pin: sFLASH CS pin */
    GPIO_InitStructure.GPIO_Pin = sFLASH_MEM_CS_GPIO_PIN;
    GPIO_Init(sFLASH_MEM_CS_GPIO_PORT, &GPIO_InitStructure);
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

    /* sFLASH_MEM_CS_GPIO, sFLASH_SPI_MOSI_GPIO, sFLASH_SPI_MISO_GPIO
	   and sFLASH_SPI_SCK_GPIO Periph clock enable */
    RCC_APB2PeriphClockCmd(sFLASH_MEM_CS_GPIO_CLK | sFLASH_SPI_MOSI_GPIO_CLK | sFLASH_SPI_MISO_GPIO_CLK |
            sFLASH_SPI_SCK_GPIO_CLK, ENABLE);

    /* sFLASH_SPI Periph clock enable */
    sFLASH_SPI_CLK_CMD(sFLASH_SPI_CLK, ENABLE);

    /* Configure sFLASH_SPI pins: SCK */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_SCK_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(sFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pins: MOSI */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MOSI_GPIO_PIN;
    GPIO_Init(sFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_SPI pins: MISO */
    GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MISO_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(sFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

    /* Configure sFLASH_MEM_CS_GPIO_PIN pin: sFLASH CS pin */
    GPIO_InitStructure.GPIO_Pin = sFLASH_MEM_CS_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(sFLASH_MEM_CS_GPIO_PORT, &GPIO_InitStructure);

    /*!< Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();

    /*!< SPI configuration */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = sFLASH_SPI_BAUDRATE_PRESCALER;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(sFLASH_SPI, &SPI_InitStructure);

    sFLASH_SPI_CR = sFLASH_SPI->CR1;

    /*!< Enable the sFLASH_SPI  */
    SPI_Cmd(sFLASH_SPI, ENABLE);
}

/* Select sFLASH: Chip Select pin low */
void sFLASH_CS_LOW(void)
{
    acquire_spi_bus(BUS_OWNER_SFLASH);
    sFLASH_SPI->CR1 &= ((uint16_t)0xFFBF);
    sFLASH_SPI->CR1 = sFLASH_SPI_CR | ((uint16_t)0x0040);
    GPIO_ResetBits(sFLASH_MEM_CS_GPIO_PORT, sFLASH_MEM_CS_GPIO_PIN);
}

/* Deselect sFLASH: Chip Select pin high */
void sFLASH_CS_HIGH(void)
{
    GPIO_SetBits(sFLASH_MEM_CS_GPIO_PORT, sFLASH_MEM_CS_GPIO_PIN);
    release_spi_bus(BUS_OWNER_SFLASH);
    handle_spi_request();
}

/*******************************************************************************
 * Function Name  : USB_Disconnect_Config
 * Description    : Disconnect pin configuration
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_Disconnect_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable USB_DISCONNECT GPIO clock */
    RCC_APB2PeriphClockCmd(USB_DISCONNECT_GPIO_CLK, ENABLE);

    /* USB_DISCONNECT_GPIO_PIN used as USB pull-up */
    GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(USB_DISCONNECT_GPIO_PORT, &GPIO_InitStructure);
}

/*******************************************************************************
 * Function Name  : Set_USBClock
 * Description    : Configures USB Clock input (48MHz)
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Set_USBClock(void)
{
    /* Select USBCLK source */
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

    /* Enable the USB clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/*******************************************************************************
 * Function Name  : Enter_LowPowerMode
 * Description    : Power-off system clocks and power while entering suspend mode
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Enter_LowPowerMode(void)
{
    /* Set the device state to suspend */
    bDeviceState = SUSPENDED;
}

/*******************************************************************************
 * Function Name  : Leave_LowPowerMode
 * Description    : Restores system clocks and power while exiting suspend mode
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Leave_LowPowerMode(void)
{
    const DEVICE_INFO *pInfo = &Device_Info;

    /* Set the device state to the correct state */
    if (pInfo->Current_Configuration != 0)
    {
        /* Device configured */
        bDeviceState = CONFIGURED;
    }
    else
    {
        bDeviceState = ATTACHED;
    }
}

/*******************************************************************************
 * Function Name  : USB_Interrupts_Config
 * Description    : Configures the USB interrupts
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_Interrupts_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the USB interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_LP_IRQ_PRIORITY;			//OLD: 0x01
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;								//OLD: 0x00
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

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
        GPIO_ResetBits(USB_DISCONNECT_GPIO_PORT, USB_DISCONNECT_GPIO_PIN);
    }
    else
    {
        GPIO_SetBits(USB_DISCONNECT_GPIO_PORT, USB_DISCONNECT_GPIO_PIN);
    }
}

void Load_SystemFlags(void)
{
    uint32_t Address = SYSTEM_FLAGS_ADDRESS;

    if(!USE_SYSTEM_FLAGS)
        return;

    Bootloader_Version_SysFlag = (*(__IO uint16_t*) Address);
    Address += 2;

    NVMEM_SPARK_Reset_SysFlag = (*(__IO uint16_t*) Address);
    Address += 2;

    FLASH_OTA_Update_SysFlag = (*(__IO uint16_t*) Address);
    Address += 2;

    OTA_FLASHED_Status_SysFlag = (*(__IO uint16_t*) Address);
    Address += 2;

    Factory_Reset_SysFlag = (*(__IO uint16_t*) Address);
    Address += 2;

    dfu_on_no_firmware = (*(__IO uint8_t*) Address++);
    Factory_Reset_Done_SysFlag = (*(__IO uint8_t*) Address++);
    StartupMode_SysFlag = (*(__IO uint8_t*) Address++);
    FeaturesEnabled_SysFlag = (*(__IO uint8_t*) Address++);

    RCC_CSR_SysFlag = (*(__IO uint32_t*) Address);
    Address += 4;
}

void Save_SystemFlags(void)
{
    uint32_t Address = SYSTEM_FLAGS_ADDRESS;
    FLASH_Status FLASHStatus = FLASH_COMPLETE;

    if(!USE_SYSTEM_FLAGS)
        return;

    /* Unlock the Flash Program Erase Controller */
    FLASH_Unlock();

    /* Clear All pending flags */
    FLASH_ClearFlags();

    /* Erase the Internal Flash pages */
    FLASHStatus = FLASH_ErasePage(SYSTEM_FLAGS_ADDRESS);
    while(FLASHStatus != FLASH_COMPLETE);

    /* Program Bootloader_Version_SysFlag */
    FLASHStatus = FLASH_ProgramHalfWord(Address, Bootloader_Version_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    /* Program NVMEM_SPARK_Reset_SysFlag */
    FLASHStatus = FLASH_ProgramHalfWord(Address, NVMEM_SPARK_Reset_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    /* Program FLASH_OTA_Update_SysFlag */
    FLASHStatus = FLASH_ProgramHalfWord(Address, FLASH_OTA_Update_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    /* Program OTA_FLASHED_Status_SysFlag */
    FLASHStatus = FLASH_ProgramHalfWord(Address, OTA_FLASHED_Status_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    /* Program Factory_Reset_SysFlag */
    FLASHStatus = FLASH_ProgramHalfWord(Address, Factory_Reset_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    FLASHStatus = FLASH_ProgramHalfWord(Address,  Factory_Reset_Done_SysFlag<<8 | dfu_on_no_firmware);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    // the flag that comes after this will be added here, shifted 8 places.
    FLASHStatus = FLASH_ProgramHalfWord(Address, FeaturesEnabled_SysFlag<<8 | StartupMode_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 2;

    //Note: using ProgramWord here.
    FLASHStatus = FLASH_ProgramWord(Address, RCC_CSR_SysFlag);
    while(FLASHStatus != FLASH_COMPLETE);
    Address += 4;

    /* Locks the FLASH Program Erase Controller */
    FLASH_Lock();
}

void FLASH_ClearFlags(void)
{
    /* Clear All pending flags */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
}

FLASH_Status FLASH_WriteProtection_Enable(uint32_t FLASH_Pages)
{
    /* Get pages write protection status */
    WRPR_Value = FLASH_GetWriteProtectionOptionByte();

    FLASH_Status status = FLASH_COMPLETE;

    /* Check if desired pages are not yet write protected */
    if(((~WRPR_Value) & FLASH_Pages ) != FLASH_Pages)
    {
        /* Get current write protected pages and the new pages to be protected */
        Flash_Pages_Protected =  (~WRPR_Value) | FLASH_Pages;

        /* Unlock the Flash Program Erase controller */
        FLASH_Unlock();

        /* Erase all the option Bytes because if a program operation is
	      performed on a protected page, the Flash memory returns a
	      protection error */
        status = FLASH_EraseOptionBytes();

        /* Enable the pages write protection */
        status = FLASH_EnableWriteProtection(Flash_Pages_Protected);

        /* Generate System Reset to load the new option byte values */
        NVIC_SystemReset();
    }
    return status;
}

FLASH_Status FLASH_WriteProtection_Disable(uint32_t FLASH_Pages)
{
    FLASH_Status status = FLASH_COMPLETE;

    /* Get pages write protection status */
    WRPR_Value = FLASH_GetWriteProtectionOptionByte();

    /* Check if desired pages are already write protected */
    if((WRPR_Value | (~FLASH_Pages)) != 0xFFFFFFFF)
    {
        /* Get pages already write protected */
        Flash_Pages_Protected = ~(WRPR_Value | FLASH_Pages);

        /* Unlock the Flash Program Erase controller */
        FLASH_Unlock();

        /* Erase all the option Bytes */
        status = FLASH_EraseOptionBytes();

        /* Check if there is write protected pages */
        if(Flash_Pages_Protected != 0x0 && status==FLASH_COMPLETE)
        {
            /* Restore write protected pages */
            status = FLASH_EnableWriteProtection(Flash_Pages_Protected);
        }

        /* Generate System Reset to load the new option byte values */
        NVIC_SystemReset();
    }
    return status;
}

void FLASH_Erase_Impl(uint32_t address, uint32_t length)
{
    FLASH_Status status = FLASH_COMPLETE;

    /* Unlock the Flash Program Erase Controller */
    FLASH_Unlock();

    unsigned EraseCounter;
    /* Define the number of Internal Flash pages to be erased */
    unsigned NbrOfPage = length / INTERNAL_FLASH_PAGE_SIZE;

    /* Clear All pending flags */
    FLASH_ClearFlags();

    /* Erase the Internal Flash pages */
    for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (status == FLASH_COMPLETE); EraseCounter++)
    {
        status = FLASH_ErasePage(CORE_FW_ADDRESS + (INTERNAL_FLASH_PAGE_SIZE * EraseCounter));
    }

    /* Locks the FLASH Program Erase Controller */
    FLASH_Lock();
}

void FLASH_Erase()
{
    FLASH_Erase_Impl(CORE_FW_ADDRESS, (INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS));
}

void FLASH_Backup(uint32_t sFLASH_Address)
{
    /* Initialize SPI Flash */
    sFLASH_Init();

    /* Define the number of External Flash pages to be erased */
    unsigned NbrOfPage = EXTERNAL_FLASH_BLOCK_SIZE / sFLASH_PAGESIZE;
    unsigned EraseCounter;

    /* Erase the SPI Flash pages */
    for (EraseCounter = 0; (EraseCounter < NbrOfPage); EraseCounter++)
    {
        sFLASH_EraseSector(sFLASH_Address + (sFLASH_PAGESIZE * EraseCounter));
    }

    uint32_t Internal_Flash_Address = CORE_FW_ADDRESS;
    uint32_t External_Flash_Address = sFLASH_Address;
    uint8_t External_Flash_Data[4];
    /* Program External Flash */
    while (Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS)
    {
        /* Read data from Internal Flash memory */
        uint32_t Internal_Flash_Data = (*(__IO uint32_t*) Internal_Flash_Address);
        Internal_Flash_Address += 4;

        /* Program Word to SPI Flash memory */
        External_Flash_Data[0] = (uint8_t)(Internal_Flash_Data & 0xFF);
        External_Flash_Data[1] = (uint8_t)((Internal_Flash_Data & 0xFF00) >> 8);
        External_Flash_Data[2] = (uint8_t)((Internal_Flash_Data & 0xFF0000) >> 16);
        External_Flash_Data[3] = (uint8_t)((Internal_Flash_Data & 0xFF000000) >> 24);
        //OR
        //*((uint32_t *)External_Flash_Data) = Internal_Flash_Data;
        sFLASH_WriteBuffer(External_Flash_Data, External_Flash_Address, 4);
        External_Flash_Address += 4;
    }
}

void FLASH_Restore(uint32_t sFLASH_Address)
{
    /* Initialize SPI Flash */
    sFLASH_Init();

    FLASH_Erase();

    uint32_t Internal_Flash_Address = CORE_FW_ADDRESS;
    uint32_t External_Flash_Address = sFLASH_Address;

    /* Unlock the Flash Program Erase Controller */
    FLASH_Unlock();

    FLASH_Status status = FLASH_COMPLETE;
    uint8_t External_Flash_Data[4];
    /* Program Internal Flash Bank1 */
    while ((Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS) && (status == FLASH_COMPLETE))
    {
        /* Read data from SPI Flash memory */
        sFLASH_ReadBuffer(External_Flash_Data, External_Flash_Address, 4);
        External_Flash_Address += 4;

        /* Program Word to Internal Flash memory */
        uint32_t Internal_Flash_Data = (uint32_t)(External_Flash_Data[0] | (External_Flash_Data[1] << 8) | (External_Flash_Data[2] << 16) | (External_Flash_Data[3] << 24));
        //OR
        //Internal_Flash_Data = *((uint32_t *)External_Flash_Data);
        status = FLASH_ProgramWord(Internal_Flash_Address, Internal_Flash_Data);
        Internal_Flash_Address += 4;
    }

    /* Locks the FLASH Program Erase Controller */
    FLASH_Lock();
}

uint32_t FLASH_PagesMask(uint32_t fileSize)
{
    //Calculate the number of flash pages that needs to be erased
    return (fileSize+sFLASH_PAGESIZE-1)/sFLASH_PAGESIZE;
}

void FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize)
{
    OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();

    /* Define the number of External Flash pages to be erased */
    unsigned NbrOfPage = FLASH_PagesMask(fileSize);
    unsigned EraseCounter;
    /* Erase the SPI Flash pages */
    for (EraseCounter = 0; (EraseCounter < NbrOfPage); EraseCounter++)
    {
        sFLASH_EraseSector(sFLASH_Address + (sFLASH_PAGESIZE * EraseCounter));
    }
}

int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize)
{
    const uint8_t *writeBuffer = pBuffer;
    uint8_t readBuffer[bufferSize];

    // ensure different for first write
    readBuffer[0] = writeBuffer[0]+1;
    unsigned i;
    for (i=50; i-->0 && memcmp(writeBuffer, readBuffer, bufferSize); i++)
    {
        /* Write Data Buffer to SPI Flash memory */
        sFLASH_WriteBuffer(writeBuffer, address, bufferSize);

        /* Read Data Buffer from SPI Flash memory */
        sFLASH_ReadBuffer(readBuffer, address, bufferSize);
    }
    return !i;  // return 0 (success) if i!=0
}

void FLASH_End(void)
{
    FLASH_OTA_Update_SysFlag = 0x0005;
    Save_SystemFlags();

    BKP_WriteBackupRegister(BKP_DR10, 0x0005);
}

// buf length must be at least EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH
void FLASH_Read_ServerAddress_Data(void *buf)
{
    sFLASH_ReadBuffer(buf,
            EXTERNAL_FLASH_SERVER_DOMAIN_ADDRESS,
            EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH);
}

// buf length must be at least EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH
void FLASH_Write_ServerAddress_Data(const uint8_t *buf)
{
    sFLASH_WriteBuffer(buf,
            EXTERNAL_FLASH_SERVER_DOMAIN_ADDRESS,
            EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH);
}

// keyBuffer length must be at least EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH
void FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    sFLASH_ReadBuffer(keyBuffer,
            EXTERNAL_FLASH_SERVER_PUBLIC_KEY_ADDRESS,
            EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
}

// keyBuffer length must be at least EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH
void FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer)
{
    sFLASH_WriteBuffer(keyBuffer,
            EXTERNAL_FLASH_SERVER_PUBLIC_KEY_ADDRESS,
            EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);
}


// keyBuffer length must be at least EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH
void FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{
    sFLASH_ReadBuffer(keyBuffer,
            EXTERNAL_FLASH_CORE_PRIVATE_KEY_ADDRESS,
            EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);
}

bool FACTORY_Flash_Reset(void)
{
    // Restore the Factory programmed application firmware from External Flash
    FLASH_Restore(EXTERNAL_FLASH_FAC_ADDRESS);

    Factory_Reset_SysFlag = 0xFFFF;
    OTA_FLASHED_Status_SysFlag = 0x0000;
    dfu_on_no_firmware = 0;
    Factory_Reset_Done_SysFlag = 0x5A;
    Finish_Update();

    return true;//This won't be reached
}

void BACKUP_Flash_Reset(void)
{
    //Restore the Backup programmed application firmware from External Flash
    FLASH_Restore(EXTERNAL_FLASH_BKP_ADDRESS);

    OTA_FLASHED_Status_SysFlag = 0x0000;
    dfu_on_no_firmware = 0;

    Finish_Update();
}

void OTA_Flash_Reset(void)
{
    //First take backup of the current application firmware to External Flash
    FLASH_Backup(EXTERNAL_FLASH_BKP_ADDRESS);

    FLASH_OTA_Update_SysFlag = 0x5555;
    Save_SystemFlags();
    BKP_WriteBackupRegister(BKP_DR10, 0x5555);

    //Restore the OTA programmed application firmware from External Flash
    FLASH_Restore(EXTERNAL_FLASH_OTA_ADDRESS);

    OTA_FLASHED_Status_SysFlag = 0x0001;

    Finish_Update();
}

bool OTA_Flashed_GetStatus(void)
{
    if(OTA_FLASHED_Status_SysFlag == 0x0001)
        return true;
    else
        return false;
}

void OTA_Flashed_ResetStatus(void)
{
    OTA_FLASHED_Status_SysFlag = 0x0000;
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
    FLASH_OTA_Update_SysFlag = 0x5000;
    Save_SystemFlags();

    BKP_WriteBackupRegister(BKP_DR10, 0x5000);

    USB_Cable_Config(DISABLE);

    NVIC_SystemReset();
}

uint16_t Bootloader_Get_Version(void)
{
    return Bootloader_Version_SysFlag;
}

void Bootloader_Update_Version(uint16_t bootloaderVersion)
{
    Bootloader_Version_SysFlag = bootloaderVersion;
    Save_SystemFlags();
}
