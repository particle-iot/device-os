/**
 ******************************************************************************
 * @file    hw_config.c
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Hardware Configuration & Setup
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_pwr.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
int8_t OTA_UPDATE_MODE = 0;		//0, -1, 1
uint8_t DFU_DEVICE_MODE = 0;	//0, 1
uint8_t FACTORY_RESET_MODE = 0;	//0, 1
uint8_t USE_SYSTEM_FLAGS = 0;	//0, 1

__IO uint32_t TimingDelay;
__IO uint32_t TimingLED;
__IO uint32_t TimingBUTTON;
__IO uint32_t TimingIWDGReload;

__IO uint8_t IWDG_SYSTEM_RESET;

GPIO_TypeDef* DIO_PORT[] = {D0_GPIO_PORT, D1_GPIO_PORT, D2_GPIO_PORT, D3_GPIO_PORT,
								D4_GPIO_PORT, D5_GPIO_PORT, D6_GPIO_PORT, D7_GPIO_PORT};
const uint16_t DIO_PIN[] = {D0_PIN, D1_PIN, D2_PIN, D3_PIN,
								D4_PIN, D5_PIN, D6_PIN, D7_PIN};
const uint32_t DIO_CLK[] = {D0_GPIO_CLK, D1_GPIO_CLK, D2_GPIO_CLK, D3_GPIO_CLK,
								D4_GPIO_CLK, D5_GPIO_CLK, D6_GPIO_CLK, D7_GPIO_CLK};

GPIO_TypeDef* LED_PORT[] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT, LED4_GPIO_PORT};
const uint16_t LED_PIN[] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};
const uint32_t LED_CLK[] = {LED1_GPIO_CLK, LED2_GPIO_CLK, LED3_GPIO_CLK, LED4_GPIO_CLK};
__IO uint16_t LED_TIM_CCR[] = {0x0000, 0x0000, 0x0000, 0x0000};
int8_t delta1, delta2, delta3;

GPIO_TypeDef* BUTTON_PORT[] = {BUTTON1_GPIO_PORT, BUTTON2_GPIO_PORT};
const uint16_t BUTTON_PIN[] = {BUTTON1_PIN, BUTTON2_PIN};
const uint32_t BUTTON_CLK[] = {BUTTON1_GPIO_CLK, BUTTON2_GPIO_CLK};
GPIOMode_TypeDef BUTTON_GPIO_MODE[] = {BUTTON1_GPIO_MODE, BUTTON2_GPIO_MODE};
__IO uint16_t BUTTON_DEBOUNCED_TIME[] = {0, 0};

const uint16_t BUTTON_EXTI_LINE[] = {BUTTON1_EXTI_LINE, BUTTON2_EXTI_LINE};
const uint16_t BUTTON_PORT_SOURCE[] = {BUTTON1_EXTI_PORT_SOURCE, BUTTON2_EXTI_PORT_SOURCE};
const uint16_t BUTTON_PIN_SOURCE[] = {BUTTON1_EXTI_PIN_SOURCE, BUTTON2_EXTI_PIN_SOURCE};
const uint16_t BUTTON_IRQn[] = {BUTTON1_EXTI_IRQn, BUTTON2_EXTI_IRQn};
EXTITrigger_TypeDef BUTTON_EXTI_TRIGGER[] = {BUTTON1_EXTI_TRIGGER, BUTTON2_EXTI_TRIGGER};

uint16_t CORE_FW_Version_SysFlag = 0xFFFF;
uint16_t NVMEM_SPARK_Reset_SysFlag = 0xFFFF;
uint16_t FLASH_OTA_Update_SysFlag = 0xFFFF;

uint32_t Internal_Flash_Address = 0;
uint32_t External_Flash_Address = 0;
uint32_t Internal_Flash_Data = 0;
uint8_t External_Flash_Data[4];
uint32_t EraseCounter = 0;
uint32_t NbrOfPage = 0;
volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

__IO uint16_t CC3000_SPI_CR;
__IO uint16_t sFLASH_SPI_CR;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
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

	/* NVIC configuration */
	NVIC_Configuration();

    /* Enable AFIO clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/* Configure DIOs */
	int Dx;
	for(Dx = 0; Dx < Dn; ++Dx)
	{
		DIO_Init(Dx);
	}

	/* Configure TIM1 for LED-PWM and BUTTON-DEBOUNCE usage */
	UI_Timer_Configure();

	/* Configure the LEDs and set the default states */
	int LEDx;
	for(LEDx = 0; LEDx < LEDn; ++LEDx)
	{
	    LED_Init(LEDx);
	}

    /* Configure the Button */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);

	/* Setup SysTick Timer for 1 msec interrupts */
	if (SysTick_Config(SystemCoreClock / 1000))
	{
		/* Capture error */
		while (1)
		{
		}
	}

	/* Configure the SysTick Handler Priority: Preemption priority and subpriority */
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x03, 0x00));
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
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* Configure the Priority Group to 2 bits */
	/* 2 bits for pre-emption priority(0-3 PreemptionPriority) and 2 bits for subpriority(0-3 SubPriority) */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

/*******************************************************************************
* Function Name  : Delay
* Description    : Inserts a delay time.
* Input          : nTime: specifies the delay time length, in milliseconds.
* Output         : None
* Return         : None
*******************************************************************************/
void Delay(uint32_t nTime)
{
    TimingDelay = nTime;

    while (TimingDelay != 0x00);
}

#if defined (USE_SPARK_CORE_V02)
void RTC_Configuration(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the RTC Alarm Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Check if the StandBy flag is set */
	if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
	{
		/* System resumed from STANDBY mode */

		/* Clear StandBy flag */
		PWR_ClearFlag(PWR_FLAG_SB);

		/* Wait for RTC APB registers synchronisation */
		RTC_WaitForSynchro();

		BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

		/* No need to configure the RTC as the RTC configuration(clock source, enable,
	       prescaler,...) is kept after wake-up from STANDBY */
	}
	else
	{
		/* StandBy flag is not set */

		if(BKP_ReadBackupRegister(BKP_DR9) == 0xA5A5)
		{
			/* Request to enter STANDBY mode (Wake Up flag is cleared in PWR_EnterSTANDBYMode function) */
			PWR_EnterSTANDBYMode();
		}

		/* Enable LSE */
		RCC_LSEConfig(RCC_LSE_ON);

		/* Wait till LSE is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
		{
			//Do nothing
		}

		/* Select LSE as RTC Clock Source */
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		/* Enable RTC Clock */
		RCC_RTCCLKCmd(ENABLE);

		/* Wait for RTC registers synchronization */
		RTC_WaitForSynchro();

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Enable the RTC Second and RTC Alarm interrupt */
		RTC_ITConfig(RTC_IT_SEC | RTC_IT_ALR, ENABLE);

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Set RTC prescaler: set RTC period to 1sec */
		RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
}

void Enter_STANDBY_Mode(void)
{
	BKP_WriteBackupRegister(BKP_DR9, 0xA5A5);

    NVIC_SystemReset();
}
#endif

void IWDG_Reset_Enable(uint32_t msTimeout)
{
	uint16_t Reload_Value;

	if(msTimeout > 10000)
		msTimeout = 10000;	//Max IWDG timeout that can be set is 10 sec

	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG counter clock: LSI/256 */
	IWDG_SetPrescaler(IWDG_Prescaler_256);

	/* IWDG timeout may vary due to LSI frequency dispersion */
	Reload_Value = (uint16_t)((msTimeout * 40) / 256); //Assuming LSI Frequency = 40000

	IWDG_SetReload(Reload_Value);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();
}

/**
  * @brief  Configures Dx GPIO.
  * @param  Dx: Specifies the Dx to be configured.
  * @retval None
  */
void DIO_Init(DIO_TypeDef Dx)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO_Dx Clock */
    RCC_APB2PeriphClockCmd(DIO_CLK[Dx], ENABLE);

    /* Configure the GPIO_Dx pin */
    GPIO_InitStructure.GPIO_Pin = DIO_PIN[Dx];
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DIO_PORT[Dx], &GPIO_InitStructure);

    /* Set to Off State */
    DIO_SetState(Dx, LOW);
}

/**
  * @brief  Turns selected Dx On/Off.
  * @param  Dx: Specifies the Dx.
  * @param  State: Set On or Off.
  * @retval None
  */
DIO_Error_TypeDef DIO_SetState(DIO_TypeDef Dx, DIO_State_TypeDef State)
{
	if(Dx < 0 || Dx > Dn)
		return FAIL;
	else if(State == HIGH)
		DIO_PORT[Dx]->BSRR = DIO_PIN[Dx];
	else if(State == LOW)
		DIO_PORT[Dx]->BRR = DIO_PIN[Dx];

	return OK;
}

void UI_Timer_Configure(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* Enable TIM1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    /* TIM1 Update Frequency = 72000000/7200/100 = 100Hz = 10ms */
    /* TIM1_Prescaler: 7199 */
    /* TIM1_Autoreload: 99 -> 100Hz = 10ms */
    uint16_t TIM1_Prescaler = (SystemCoreClock / 10000) - 1;
    uint16_t TIM1_Autoreload = (10000 / UI_TIMER_FREQUENCY) - 1;

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    /* Time Base Configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
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
#if defined (USE_SPARK_CORE_V01)
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
#elif defined (USE_SPARK_CORE_V02)
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
#endif
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

#if defined (USE_SPARK_CORE_V02)
void LED_SetRGBColor(uint32_t RGB_Color)
{
	LED_TIM_CCR[2] = (uint16_t)(((RGB_Color & 0xFF0000) >> 16) * (TIM1->ARR + 1) / 255);	//LED3 -> Red Led
	LED_TIM_CCR[3] = (uint16_t)(((RGB_Color & 0xFF00) >> 8) * (TIM1->ARR + 1) / 255);		//LED4 -> Green Led
	LED_TIM_CCR[1] = (uint16_t)((RGB_Color & 0xFF) * (TIM1->ARR + 1) / 255);				//LED2 -> Blue Led
}
#endif

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
    RCC_APB2PeriphClockCmd(LED_CLK[Led], ENABLE);

    /* Configure the GPIO_LED pin as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = LED_PIN[Led];
#if defined (USE_SPARK_CORE_V01)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#elif defined (USE_SPARK_CORE_V02)
    if(Led == LED_USER)
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    else
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(LED_PORT[Led], &GPIO_InitStructure);
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_USER, LED_RGB
  * @retval None
  */
void LED_On(Led_TypeDef Led)
{
#if defined (USE_SPARK_CORE_V01)
	switch(Led)
	{
	case LED1:
		TIM1->CCR1 = TIM1->ARR + 1;
		break;

	case LED2:
		TIM1->CCR2 = TIM1->ARR + 1;
		break;
	}
#elif defined (USE_SPARK_CORE_V02)
	switch(Led)
	{
	case LED_USER:
		LED_PORT[Led]->BSRR = LED_PIN[Led];
		break;

	case LED_RGB:	//LED_SetRGBColor() should be called first for this Case
		TIM1->CCR2 = LED_TIM_CCR[2];
		TIM1->CCR3 = LED_TIM_CCR[3];
		TIM1->CCR1 = LED_TIM_CCR[1];
		break;
	}
#endif
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_USER, LED_RGB
  * @retval None
  */
void LED_Off(Led_TypeDef Led)
{
#if defined (USE_SPARK_CORE_V01)
	switch(Led)
	{
	case LED1:
		TIM1->CCR1 = 0;
		break;

	case LED2:
		TIM1->CCR2 = 0;
		break;
	}
#elif defined (USE_SPARK_CORE_V02)
	switch(Led)
	{
	case LED_USER:
		LED_PORT[Led]->BRR = LED_PIN[Led];
		break;

	case LED_RGB:
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
		TIM1->CCR1 = 0;
		break;
	}
#endif
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_USER, LED_RGB
  * @retval None
  */
void LED_Toggle(Led_TypeDef Led)
{
#if defined (USE_SPARK_CORE_V01)
	switch(Led)
	{
	case LED1:
		TIM1->CCR1 ^= TIM1->ARR + 1;
		break;

	case LED2:
		TIM1->CCR2 ^= TIM1->ARR + 1;
		break;
	}
#elif defined (USE_SPARK_CORE_V02)
	switch(Led)
	{
	case LED_USER:
		LED_PORT[Led]->ODR ^= LED_PIN[Led];
		break;

	case LED_RGB://LED_SetRGBColor() and LED_On() should be called first for this Case
		TIM1->CCR2 ^= LED_TIM_CCR[2];
		TIM1->CCR3 ^= LED_TIM_CCR[3];
		TIM1->CCR1 ^= LED_TIM_CCR[1];
		break;
	}
#endif
}

/**
  * @brief  Fades selected LED.
  * @param  Led: Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_RGB
  * @retval None
  */
void LED_Fade(Led_TypeDef Led)
{
#if defined (USE_SPARK_CORE_V01)
	switch(Led)
	{
	case LED1:
		if(LED_TIM_CCR[1] == 0)
			delta1 = 0;
		else if(TIM1->CCR1 == 0)
			delta1 = 1;
		else if(TIM1->CCR1 == TIM1->ARR + 1)
			delta1 = -1;

		TIM1->CCR1 += delta1;
		break;

	case LED2:
		if(LED_TIM_CCR[2] == 0)
			delta2 = 0;
		else if(TIM1->CCR2 == 0)
			delta2 = 1;
		else if(TIM1->CCR2 == TIM1->ARR + 1)
			delta2 = -1;

		TIM1->CCR2 += delta2;
		break;
	}
#elif defined (USE_SPARK_CORE_V02)
	if(Led == LED_RGB)
	{
		if(LED_TIM_CCR[2] == 0)
			delta2 = 0;
		else if(TIM1->CCR2 == 0)
			delta2 = 1;
		else if(TIM1->CCR2 == TIM1->ARR + 1)
			delta2 = -1;

		if(LED_TIM_CCR[3] == 0)
			delta3 = 0;
		else if(TIM1->CCR3 == 0)
			delta3 = 1;
		else if(TIM1->CCR3 == TIM1->ARR + 1)
			delta3 = -1;

		if(LED_TIM_CCR[1] == 0)
			delta1 = 0;
		else if(TIM1->CCR1 == 0)
			delta1 = 1;
		else if(TIM1->CCR1 == TIM1->ARR + 1)
			delta1 = -1;

		TIM1->CCR2 += delta2;
		TIM1->CCR3 += delta3;
		TIM1->CCR1 += delta1;
	}
#endif
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
    RCC_APB2PeriphClockCmd(BUTTON_CLK[Button] | RCC_APB2Periph_AFIO, ENABLE);

    /* Configure Button pin as input floating */
    GPIO_InitStructure.GPIO_Mode = BUTTON_GPIO_MODE[Button];
    GPIO_InitStructure.GPIO_Pin = BUTTON_PIN[Button];
    GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStructure);

    if (Button_Mode == BUTTON_MODE_EXTI)
    {
    	/* Disable TIM1 CC4 Interrupt */
        TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);

        /* Enable the TIM1 Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

        NVIC_Init(&NVIC_InitStructure);

        /* Enable the Button EXTI Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = BUTTON_IRQn[Button];
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

        NVIC_Init(&NVIC_InitStructure);

        BUTTON_EXTI_Config(Button, ENABLE);
    }
}

void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState)
{
    EXTI_InitTypeDef EXTI_InitStructure;

	/* Connect Button EXTI Line to Button GPIO Pin */
    GPIO_EXTILineConfig(BUTTON_PORT_SOURCE[Button], BUTTON_PIN_SOURCE[Button]);

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
    return GPIO_ReadInputDataBit(BUTTON_PORT[Button], BUTTON_PIN[Button]);
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
	GPIO_InitStructure.GPIO_Pin = CC3000_WIFI_CS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(CC3000_WIFI_CS_GPIO_PORT, &GPIO_InitStructure);

	/* Deselect CC3000 */
	CC3000_CS_HIGH();

	/* Configure CC3000_WIFI pins: Enable */
	GPIO_InitStructure.GPIO_Pin = CC3000_WIFI_EN_PIN;
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
	GPIO_InitStructure.GPIO_Pin = CC3000_SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(CC3000_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

	/* Configure CC3000_SPI pins: MOSI */
	GPIO_InitStructure.GPIO_Pin = CC3000_SPI_MOSI_PIN;
	GPIO_Init(CC3000_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

	/* Configure CC3000_SPI pins: MISO */
	GPIO_InitStructure.GPIO_Pin = CC3000_SPI_MISO_PIN;
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
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
	CC3000_SPI->CR1 &= ((uint16_t)0xFFBF);
	sFLASH_CS_HIGH();
	CC3000_SPI->CR1 = CC3000_SPI_CR | ((uint16_t)0x0040);
	GPIO_ResetBits(CC3000_WIFI_CS_GPIO_PORT, CC3000_WIFI_CS_PIN);
}

/* Deselect CC3000: ChipSelect pin high */
void CC3000_CS_HIGH(void)
{
	GPIO_SetBits(CC3000_WIFI_CS_GPIO_PORT, CC3000_WIFI_CS_PIN);
}

/* CC3000 Hardware related callbacks passed to wlan_init */
long CC3000_Read_Interrupt_Pin(void)
{
	return GPIO_ReadInputDataBit(CC3000_WIFI_INT_GPIO_PORT, CC3000_WIFI_INT_PIN );
}

void CC3000_Interrupt_Enable(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* CC3000_WIFI_INT_GPIO and AFIO Peripheral clock enable */
	RCC_APB2PeriphClockCmd(CC3000_WIFI_INT_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

	/* Configure CC3000_WIFI pins: Interrupt */
	GPIO_InitStructure.GPIO_Pin = CC3000_WIFI_INT_PIN;
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
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
		GPIO_SetBits(CC3000_WIFI_EN_GPIO_PORT, CC3000_WIFI_EN_PIN );
	}
	else
	{
		GPIO_ResetBits(CC3000_WIFI_EN_GPIO_PORT, CC3000_WIFI_EN_PIN );
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
	GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(sFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

	/* Configure sFLASH_SPI pins: MISO */
	GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MISO_PIN;
	GPIO_Init(sFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

	/* Configure sFLASH_SPI pins: MOSI */
	GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MOSI_PIN;
	GPIO_Init(sFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

	/* Configure sFLASH_MEM_CS_PIN pin: sFLASH CS pin */
	GPIO_InitStructure.GPIO_Pin = sFLASH_MEM_CS_PIN;
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
	GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(sFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

	/* Configure sFLASH_SPI pins: MOSI */
	GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MOSI_PIN;
	GPIO_Init(sFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

	/* Configure sFLASH_SPI pins: MISO */
	GPIO_InitStructure.GPIO_Pin = sFLASH_SPI_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(sFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

	/* Configure sFLASH_MEM_CS_PIN pin: sFLASH CS pin */
	GPIO_InitStructure.GPIO_Pin = sFLASH_MEM_CS_PIN;
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
	sFLASH_SPI->CR1 &= ((uint16_t)0xFFBF);
	CC3000_CS_HIGH();
	sFLASH_SPI->CR1 = sFLASH_SPI_CR | ((uint16_t)0x0040);
	GPIO_ResetBits(sFLASH_MEM_CS_GPIO_PORT, sFLASH_MEM_CS_PIN);
}

/* Deselect sFLASH: Chip Select pin high */
void sFLASH_CS_HIGH(void)
{
	GPIO_SetBits(sFLASH_MEM_CS_GPIO_PORT, sFLASH_MEM_CS_PIN);
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

	/* USB_DISCONNECT_PIN used as USB pull-up */
	GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
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
	DEVICE_INFO *pInfo = &Device_Info;

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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
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
		GPIO_ResetBits(USB_DISCONNECT_GPIO_PORT, USB_DISCONNECT_PIN);
	}
	else
	{
		GPIO_SetBits(USB_DISCONNECT_GPIO_PORT, USB_DISCONNECT_PIN);
	}
}

void Load_SystemFlags(void)
{
	uint32_t Address = SYSTEM_FLAGS_ADDRESS;

	if(!USE_SYSTEM_FLAGS)
		return;

	CORE_FW_Version_SysFlag = (*(__IO uint16_t*) Address);
	Address += 2;

	NVMEM_SPARK_Reset_SysFlag = (*(__IO uint16_t*) Address);
	Address += 2;

	FLASH_OTA_Update_SysFlag = (*(__IO uint16_t*) Address);
	Address += 2;
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
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	/* Erase the Internal Flash pages */
	FLASHStatus = FLASH_ErasePage(SYSTEM_FLAGS_ADDRESS);
	while(FLASHStatus != FLASH_COMPLETE);

	/* Program CORE_FW_Version_SysFlag */
	FLASHStatus = FLASH_ProgramHalfWord(Address, CORE_FW_Version_SysFlag);
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

	/* Locks the FLASH Program Erase Controller */
	FLASH_Lock();
}

void FLASH_Backup(uint32_t sFLASH_Address)
{
#ifdef SPARK_SFLASH_ENABLE

    /* Initialize SPI Flash */
	sFLASH_Init();

	/* Define the number of External Flash pages to be erased */
	NbrOfPage = EXTERNAL_FLASH_BLOCK_SIZE / sFLASH_PAGESIZE;

	/* Erase the SPI Flash pages */
	for (EraseCounter = 0; (EraseCounter < NbrOfPage); EraseCounter++)
	{
		sFLASH_EraseSector(sFLASH_Address + (sFLASH_PAGESIZE * EraseCounter));
	}

	Internal_Flash_Address = CORE_FW_ADDRESS;
	External_Flash_Address = sFLASH_Address;

	/* Program External Flash */
	while (Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS)
	{
	    /* Read data from Internal Flash memory */
		Internal_Flash_Data = (*(__IO uint32_t*) Internal_Flash_Address);
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

#endif
}

void FLASH_Restore(uint32_t sFLASH_Address)
{
#ifdef SPARK_SFLASH_ENABLE

#if defined (USE_SPARK_CORE_V02)
	if(FACTORY_RESET_MODE)
		LED_SetRGBColor(RGB_COLOR_WHITE);
	else if(OTA_UPDATE_MODE)
		LED_SetRGBColor(RGB_COLOR_MAGENTA);
	else
		LED_SetRGBColor(RGB_COLOR_RED);

    LED_On(LED_RGB);
#endif

    /* Initialize SPI Flash */
	sFLASH_Init();

	FLASHStatus = FLASH_COMPLETE;

	/* Unlock the Flash Program Erase Controller */
	FLASH_Unlock();

	/* Define the number of Internal Flash pages to be erased */
	NbrOfPage = (INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS) / INTERNAL_FLASH_PAGE_SIZE;

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	/* Erase the Internal Flash pages */
	for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
	{
		FLASHStatus = FLASH_ErasePage(CORE_FW_ADDRESS + (INTERNAL_FLASH_PAGE_SIZE * EraseCounter));
	}

	Internal_Flash_Address = CORE_FW_ADDRESS;
	External_Flash_Address = sFLASH_Address;

	/* Program Internal Flash Bank1 */
	while ((Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS) && (FLASHStatus == FLASH_COMPLETE))
	{
	    /* Read data from SPI Flash memory */
	    sFLASH_ReadBuffer(External_Flash_Data, External_Flash_Address, 4);
	    External_Flash_Address += 4;

	    /* Program Word to Internal Flash memory */
	    Internal_Flash_Data = (uint32_t)(External_Flash_Data[0] | (External_Flash_Data[1] << 8) | (External_Flash_Data[2] << 16) | (External_Flash_Data[3] << 24));
	    //OR
	    //Internal_Flash_Data = *((uint32_t *)External_Flash_Data);
		FLASHStatus = FLASH_ProgramWord(Internal_Flash_Address, Internal_Flash_Data);
		Internal_Flash_Address += 4;
	}

	/* Locks the FLASH Program Erase Controller */
	FLASH_Lock();

	Reset_Device();

#endif
}

void FLASH_Begin(uint32_t sFLASH_Address)
{
#ifdef SPARK_SFLASH_ENABLE

#if defined (USE_SPARK_CORE_V02)
	LED_SetRGBColor(RGB_COLOR_MAGENTA);
    LED_On(LED_RGB);
#endif

	FLASH_OTA_Update_SysFlag = 0x5555;
	Save_SystemFlags();

	BKP_WriteBackupRegister(BKP_DR10, 0x5555);

	External_Flash_Address = sFLASH_Address;

	/* Define the number of External Flash pages to be erased */
	NbrOfPage = EXTERNAL_FLASH_BLOCK_SIZE / sFLASH_PAGESIZE;

	/* Erase the SPI Flash pages */
	for (EraseCounter = 0; (EraseCounter < NbrOfPage); EraseCounter++)
	{
		sFLASH_EraseSector(sFLASH_Address + (sFLASH_PAGESIZE * EraseCounter));
	}

#endif
}

void FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize)
{
#ifdef SPARK_SFLASH_ENABLE

	uint32_t i = bufferSize >> 2;

	/* Program External Flash */
	while (i--)
	{
		Internal_Flash_Data = *((uint32_t *)pBuffer);
		pBuffer += 4;

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

	i = bufferSize & 3;

	/* Not an aligned data */
	if (i != 0)
	{
	    /* Program the last word to SPI Flash memory */
		Internal_Flash_Data = *((uint32_t *)pBuffer);

	    /* Program Word to SPI Flash memory */
		External_Flash_Data[0] = (uint8_t)(Internal_Flash_Data & 0xFF);
		External_Flash_Data[1] = (uint8_t)((Internal_Flash_Data & 0xFF00) >> 8);
		External_Flash_Data[2] = (uint8_t)((Internal_Flash_Data & 0xFF0000) >> 16);
		External_Flash_Data[3] = (uint8_t)((Internal_Flash_Data & 0xFF000000) >> 24);
		//OR
		//*((uint32_t *)External_Flash_Data) = Internal_Flash_Data;
		sFLASH_WriteBuffer(External_Flash_Data, External_Flash_Address, 4);
	}

#if defined (USE_SPARK_CORE_V01)
	LED_Toggle(LED2);
#elif defined (USE_SPARK_CORE_V02)
	LED_Toggle(LED_RGB);
#endif

#endif
}

void FLASH_End(void)
{
#ifdef SPARK_SFLASH_ENABLE

	FLASH_OTA_Update_SysFlag = 0x0005;
	Save_SystemFlags();

	BKP_WriteBackupRegister(BKP_DR10, 0x0005);

    USB_Cable_Config(DISABLE);

	NVIC_SystemReset();

#endif
}

// keyBuffer length must be at least EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH
void FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
#ifdef SPARK_SFLASH_ENABLE

	sFLASH_ReadBuffer(keyBuffer,
			EXTERNAL_FLASH_SERVER_PUBLIC_KEY_ADDRESS,
			EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH);

#endif
}

// keyBuffer length must be at least EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH
void FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{
#ifdef SPARK_SFLASH_ENABLE

	sFLASH_ReadBuffer(keyBuffer,
			EXTERNAL_FLASH_CORE_PRIVATE_KEY_ADDRESS,
			EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH);

#endif
}

void Factory_Flash_Reset(void)
{
    //First take backup of the current application firmware to External Flash
    FLASH_Backup(EXTERNAL_FLASH_BKP_ADDRESS);

    //Restore the Factory programmed application firmware from External Flash
	FLASH_Restore(EXTERNAL_FLASH_FAC_ADDRESS);
}

void OTA_Flash_Update(void)
{
    //First take backup of the current application firmware to External Flash
    FLASH_Backup(EXTERNAL_FLASH_BKP_ADDRESS);

    //Restore the OTA programmed application firmware from External Flash
	FLASH_Restore(EXTERNAL_FLASH_OTA_ADDRESS);
}

/*******************************************************************************
* Function Name  : Reset_Device.
* Description    : Reset the device.
* Input          : None.
* Return         : None.
*******************************************************************************/
void Reset_Device(void)
{
	FLASH_OTA_Update_SysFlag = 0x5000;
	Save_SystemFlags();

	BKP_WriteBackupRegister(BKP_DR10, 0x5000);

    USB_Cable_Config(DISABLE);

    NVIC_SystemReset();
}

/**
  * @brief  Computes the 32-bit CRC of a given buffer of byte data.
  * @param  pBuffer: pointer to the buffer containing the data to be computed
  * @param  BufferSize: Size of the buffer to be computed
  * @retval 32-bit CRC
  */
uint32_t Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize)
{
	uint32_t i, j;
	uint32_t Data;

	CRC_ResetDR();

	i = bufferSize >> 2;

	while (i--)
	{
		Data = *((uint32_t *)pBuffer);
		pBuffer += 4;

		Data = __RBIT(Data);//reverse the bit order of input Data
		CRC->DR = Data;
	}

	Data = CRC->DR;
	Data = __RBIT(Data);//reverse the bit order of output Data

	i = bufferSize & 3;

	while (i--)
	{
		Data ^= (uint32_t)*pBuffer++;

		for (j = 0 ; j < 8 ; j++)
		{
			if (Data & 1)
				Data = (Data >> 1) ^ 0xEDB88320;
			else
				Data >>= 1;
		}
	}

	Data ^= 0xFFFFFFFF;

	return Data;
}

void Get_Unique_Device_ID(uint8_t *Device_ID)
{
  uint32_t Device_IDx;

  Device_IDx = *(uint32_t*)ID1;
  *Device_ID++ = (uint8_t)(Device_IDx & 0xFF);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF00) >> 8);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF0000) >> 16);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF000000) >> 24);

  Device_IDx = *(uint32_t*)ID2;
  *Device_ID++ = (uint8_t)(Device_IDx & 0xFF);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF00) >> 8);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF0000) >> 16);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF000000) >> 24);

  Device_IDx = *(uint32_t*)ID3;
  *Device_ID++ = (uint8_t)(Device_IDx & 0xFF);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF00) >> 8);
  *Device_ID++ = (uint8_t)((Device_IDx & 0xFF0000) >> 16);
  *Device_ID = (uint8_t)((Device_IDx & 0xFF000000) >> 24);
}
