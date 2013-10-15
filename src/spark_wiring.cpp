/*
 * spark_wiring.c
 *
 *  Created on: Apr 15, 2013
 *      Author: zsupalla
 *      Updated: satishgn
 */

#include "spark_wiring.h"

/*
 * Globals
 */

uint8_t adcInitFirstTime = true;
uint8_t adcChannelConfigured = NONE;

PinMode digitalPinModeSaved = (PinMode)NONE;

uint8_t serial1_enabled = false;

extern __IO uint32_t TimingMillis;

/*
 * Pin mapping
 */

STM32_Pin_Info PIN_MAP[TOTAL_PINS] =
{
/*
 * gpio_peripheral (GPIOA or GPIOB; not using GPIOC)
 * gpio_pin (0-15)
 * adc_channel (0-9 or NONE. Note we don't define the peripheral because our chip only has one)
 * timer_peripheral (TIM2 - TIM4, or NONE)
 * timer_ch (1-4, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 */
  { GPIOB, GPIO_Pin_7, NONE, TIM4, TIM_Channel_2, (PinMode)NONE },
  { GPIOB, GPIO_Pin_6, NONE, TIM4, TIM_Channel_1, (PinMode)NONE },
  { GPIOB, GPIO_Pin_5, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOB, GPIO_Pin_4, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOB, GPIO_Pin_3, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_15, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_14, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_13, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_8, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_9, NONE, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_0, ADC_Channel_0, TIM2, TIM_Channel_1, (PinMode)NONE },
  { GPIOA, GPIO_Pin_1, ADC_Channel_1, TIM2, TIM_Channel_2, (PinMode)NONE },
  { GPIOA, GPIO_Pin_4, ADC_Channel_4, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_5, ADC_Channel_5, NULL, NONE, (PinMode)NONE },
  { GPIOA, GPIO_Pin_6, ADC_Channel_6, TIM3, TIM_Channel_1, (PinMode)NONE },
  { GPIOA, GPIO_Pin_7, ADC_Channel_7, TIM3, TIM_Channel_2, (PinMode)NONE },
  { GPIOB, GPIO_Pin_0, ADC_Channel_8, TIM3, TIM_Channel_3, (PinMode)NONE },
  { GPIOB, GPIO_Pin_1, ADC_Channel_9, TIM3, TIM_Channel_4, (PinMode)NONE },
  { GPIOA, GPIO_Pin_3, ADC_Channel_3, TIM2, TIM_Channel_4, (PinMode)NONE },
  { GPIOA, GPIO_Pin_2, ADC_Channel_2, TIM2, TIM_Channel_3, (PinMode)NONE },
  { GPIOA, GPIO_Pin_10, NONE, NULL, NONE, (PinMode)NONE }
};


void serial_begin(uint32_t baudRate);
void serial_end(void);
uint8_t serial_available(void);
int32_t serial_read(void);
void serial_write(uint8_t Data);
void serial_print(const char * str);
void serial_println(const char * str);

void serial1_begin(uint32_t baudRate);
void serial1_end(void);
uint8_t serial1_available(void);
int32_t serial1_read(void);
void serial1_write(uint8_t Data);
void serial1_print(const char * str);
void serial1_println(const char * str);

/*
 * Serial Interfaces
 */

Serial_Interface Serial =
{
	serial_begin,
	serial_end,
	serial_available,
	serial_read,
	serial_write,
	serial_print,
	serial_println
};

Serial_Interface Serial1 =
{
	serial1_begin,
	serial1_end,
	serial1_available,
	serial1_read,
	serial1_write,
	serial1_print,
	serial1_println
};

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP, or INPUT_PULLDOWN
 */
void pinMode(uint16_t pin, PinMode setMode)
{

	if (pin >= TOTAL_PINS || setMode == NONE )
	{
		return;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return;
	}

	// Serial1 safety check
	if (serial1_enabled == true && (pin == RX || pin == TX))
	{
		return;
	}

	GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
	uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;

	GPIO_InitTypeDef GPIO_InitStructure;

	if (gpio_port == GPIOA )
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (gpio_port == GPIOB )
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}

	GPIO_InitStructure.GPIO_Pin = gpio_pin;

	switch (setMode)
	{

	case OUTPUT:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		PIN_MAP[pin].pin_mode = OUTPUT;
		break;

	case INPUT:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		PIN_MAP[pin].pin_mode = INPUT;
		break;

	case INPUT_PULLUP:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
		PIN_MAP[pin].pin_mode = INPUT_PULLUP;
		break;

	case INPUT_PULLDOWN:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
		PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
		break;

	case AF_OUTPUT:	//Used internally for Alternate Function Output(TIM, UART, SPI etc)
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		PIN_MAP[pin].pin_mode = AF_OUTPUT;
		break;

	case AN_INPUT:	//Used internally for ADC Input
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
		PIN_MAP[pin].pin_mode = AN_INPUT;
		break;

	default:
		break;
	}

	GPIO_Init(gpio_port, &GPIO_InitStructure);
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void digitalWrite(uint16_t pin, uint8_t value)
{
	if (pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode == INPUT
	|| PIN_MAP[pin].pin_mode == INPUT_PULLUP|| PIN_MAP[pin].pin_mode == INPUT_PULLDOWN
	|| PIN_MAP[pin].pin_mode == AN_INPUT || PIN_MAP[pin].pin_mode == NONE)
	{
		return;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return;
	}

	// Serial1 safety check
	if (serial1_enabled == true && (pin == RX || pin == TX))
	{
		return;
	}

	//If the pin is used by analogWrite, we need to change the mode
	if(PIN_MAP[pin].pin_mode == AF_OUTPUT)
	{
		pinMode(pin, OUTPUT);
	}

	if (value == HIGH)
	{
		PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin;
	}
	else if (value == LOW)
	{
		PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin;
	}
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t digitalRead(uint16_t pin)
{
	if (pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode == OUTPUT || PIN_MAP[pin].pin_mode == NONE)
	{
		return -1;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return -1;
	}

	// Serial1 safety check
	if (serial1_enabled == true && (pin == RX || pin == TX))
	{
		return -1;
	}

	if(PIN_MAP[pin].pin_mode == AN_INPUT)
	{
		if(digitalPinModeSaved == OUTPUT || digitalPinModeSaved == NONE)
		{
			return -1;
		}
		else
		{
			//Restore the PinMode after calling analogRead on same pin earlier
			pinMode(pin, digitalPinModeSaved);
		}
	}

	return GPIO_ReadInputDataBit(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin);
}

/*
 * @brief Initialize the ADC peripheral.
 */
void ADCInit()
{

	ADC_InitTypeDef ADC_InitStructure;

	// ADCCLK = PCLK2/4
	// RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	// ADCCLK = PCLK2/6 = 72/6 = 12MHz
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	// Enable ADC1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	// Put everything back to power-on defaults
	ADC_DeInit(ADC1);

	// ADC1 Configuration
	// ADC1 operate independently
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	// Disable the scan conversion so we do one at a time
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	// Don't do continuous conversions - do them on demand
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	// Start conversion by software, not an external trigger
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	// Conversions are 12 bit - put them in the lower 12 bits of the result
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	// Say how many channels would be used by the sequencer
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	// Now do the setup
	ADC_Init(ADC1, &ADC_InitStructure);

	// Enable ADC1
	ADC_Cmd(ADC1, ENABLE);

	// Enable ADC1 reset calibration register
	ADC_ResetCalibration(ADC1);

	// Check the end of ADC1 reset calibration register
	while(ADC_GetResetCalibrationStatus(ADC1));

	// Start ADC1 calibration
	ADC_StartCalibration(ADC1);

	// Check the end of ADC1 calibration
	while(ADC_GetCalibrationStatus(ADC1));
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t analogRead(uint16_t pin)
{
	// Allow people to use 0-7 to define analog pins by checking to see if the values are too low.
	if (pin < FIRST_ANALOG_PIN)
	{
		pin = pin + FIRST_ANALOG_PIN;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return -1;
	}

	// Serial1 safety check
	if (serial1_enabled == true && (pin == RX || pin == TX))
	{
		return -1;
	}

	if (pin >= TOTAL_PINS || PIN_MAP[pin].adc_channel == NONE )
	{
		return -1;
	}

	if (adcChannelConfigured != PIN_MAP[pin].adc_channel)
	{
		digitalPinModeSaved = PIN_MAP[pin].pin_mode;
		pinMode(pin, AN_INPUT);
	}

	if (adcInitFirstTime == true)
	{
		ADCInit();
		adcInitFirstTime = false;
	}

	if (adcChannelConfigured != PIN_MAP[pin].adc_channel)
	{
		ADC_RegularChannelConfig(ADC1, PIN_MAP[pin].adc_channel, 1, ADC_SAMPLING_TIME);

		adcChannelConfigured = PIN_MAP[pin].adc_channel;
	}

	//Start ADC1 Software Conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	// Wait until conversion completion
	// while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

	// Get the conversion value
	return ADC_GetConversionValue(ADC1);
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */
void analogWrite(uint16_t pin, uint8_t value)
{

	if (pin >= TOTAL_PINS || PIN_MAP[pin].timer_peripheral == NULL)
	{
		return;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return;
	}

	// Serial1 safety check
	if (serial1_enabled == true && (pin == RX || pin == TX))
	{
		return;
	}

	if(PIN_MAP[pin].pin_mode != OUTPUT && PIN_MAP[pin].pin_mode != AF_OUTPUT)
	{
		return;
	}

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	//PWM Frequency : 500 Hz
	uint16_t TIM_Prescaler = (uint16_t)(SystemCoreClock / 10000) - 1;
	uint16_t TIM_ARR = (uint16_t)(10000 / TIM_PWM_FREQ) - 1;

	uint16_t Duty_Cycle = (uint16_t)((value * 100) / 255);
	// TIM Channel Duty Cycle(%) = (TIM_CCR / TIM_ARR + 1) * 100
	uint16_t TIM_CCR = (uint16_t)((Duty_Cycle * (TIM_ARR + 1)) / 100);

	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	pinMode(pin, AF_OUTPUT);

	// TIM clock enable
	if(PIN_MAP[pin].timer_peripheral == TIM2)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	else if(PIN_MAP[pin].timer_peripheral == TIM3)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	else if(PIN_MAP[pin].timer_peripheral == TIM4)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	// Time base configuration
	TIM_TimeBaseStructure.TIM_Period = TIM_ARR;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM_Prescaler;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(PIN_MAP[pin].timer_peripheral, &TIM_TimeBaseStructure);

	// PWM1 Mode configuration
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_Pulse = TIM_CCR;

	if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
	{
		// PWM1 Mode configuration: Channel1
		TIM_OC1Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}
	else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
	{
		// PWM1 Mode configuration: Channel2
		TIM_OC2Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}
	else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
	{
		// PWM1 Mode configuration: Channel3
		TIM_OC3Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}
	else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
	{
		// PWM1 Mode configuration: Channel4
		TIM_OC4Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}

	TIM_ARRPreloadConfig(PIN_MAP[pin].timer_peripheral, ENABLE);

	// TIM enable counter
	TIM_Cmd(PIN_MAP[pin].timer_peripheral, ENABLE);
}

/*
 * TIMING
 */

/*
 * @brief Should return the number of milliseconds since the processor started up.
 * 		  This is useful for measuring the passage of time.
 * 		  For now, let's not worry about what happens when this overflows (which should happen after 49 days).
 * 		  At some point we'll have to figure that out, though.
 */
uint32_t millis()
{
	return TimingMillis;
}

/*
 * @brief This should block for a certain number of milliseconds.
 *        There are a number of ways to implement this, but I borrowed the one that Wiring/Arduino uses;
 *        Using the millis() function to check if a certain amount of time has passed.
 */
void delay(uint32_t ms)
{
	//uint32_t start = millis();
	//while(millis() - start < ms);
	//OR
	//Use the Delay() from main.c
	Delay(ms);
}

/*
 * @brief This should block for a certain number of microseconds.
 *        This will only be used for small intervals of time, so it has different requirements than the above.
 *        It must be accurate at small intervals, but does not have to function properly
 *        with intervals more than a couple of seconds.
 *
 *        The below implementation is borrowed straight from Maple. I do not understand the code, nor do I
 *        understand the derivation of the multiplier.
 *
 *        This function is lower priority than the others.
 */
void delayMicroseconds(uint32_t us)
{
	// We have to multiply this by something, but I'm not sure what.
	// Depends on how many clock cycles the below assembly code takes, I suppose.
	//
	/*
	 us *= STM32_DELAY_US_MULT;

	 // fudge for function call overhead
	 us--;
	 asm volatile("   mov r0, %[us]          \n\t"
	 "1: subs r0, #1            \n\t"
	 "   bhi 1b                 \n\t"
	 :
	 : [us] "r" (us)
	 : "r0");
	 */
}

void serial_begin(uint32_t baudRate)
{
	USB_USART_Init(baudRate);
}

void serial_end(void)
{
	//To Do
}

uint8_t serial_available(void)
{
	return USB_USART_Available_Data();
}

int32_t serial_read(void)
{
	return USB_USART_Receive_Data();
}

void serial_write(uint8_t Data)
{
	USB_USART_Send_Data(Data);
}

void serial_print(const char * str)
{
	while (*str)
	{
		serial_write(*str++);
	}
}

void serial_println(const char * str)
{
	serial_print(str);
	serial_print("\r\n");
}

void serial1_begin(uint32_t baudRate)
{
	USART_InitTypeDef USART_InitStructure;

	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// Configure USART Rx as input floating
	pinMode(RX, INPUT);

	// Configure USART Tx as alternate function push-pull
	pinMode(TX, AF_OUTPUT);

	// USART default configuration
	// USART configured as follow:
	// - BaudRate = (set baudRate as 9600 baud)
	// - Word Length = 8 Bits
	// - One Stop Bit
	// - No parity
	// - Hardware flow control disabled (RTS and CTS signals)
	// - Receive and transmit enabled
	USART_InitStructure.USART_BaudRate = baudRate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	// Configure USART
	USART_Init(USART2, &USART_InitStructure);

	// Enable the USART
	USART_Cmd(USART2, ENABLE);

	serial1_enabled = true;
}

void serial1_end(void)
{
	// Disable the USART
	USART_Cmd(USART2, DISABLE);

	serial1_enabled = false;
}

uint8_t serial1_available(void)
{
	// Check if the USART Receive Data Register is not empty
	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
		return 1;
	else
		return 0;
}

int32_t serial1_read(void)
{
	// Return the received byte
	return USART_ReceiveData(USART2);
}

void serial1_write(uint8_t Data)
{
	// Send one byte from USART
	USART_SendData(USART2, Data);

	// Loop until USART DR register is empty
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
	{
	}
}

void serial1_print(const char * str)
{
	while (*str)
	{
		serial1_write(*str++);
	}
}

void serial1_println(const char * str)
{
	serial1_print(str);
	serial1_print("\r\n");
}

