/*
 * spark_wiring.c
 *
 *  Created on: Apr 15, 2013
 *      Author: zsupalla
 */

#include "spark_wiring.h"

/*
 * Globals
 */

uint8_t adcFirstTime = true;

/*
 * Pin mapping
 */

STM32_Pin_Info PIN_MAP[TOTAL_PINS] = {
/*
 * gpio_peripheral (GPIOA or GPIOB; not using GPIOC)
 * gpio_pin (0-15)
 * adc_channel (0-9 or NONE. Note we don't define the peripheral because our chip only has one)
 * timer_peripheral (TIM1 - TIM4, or NONE)
 * timer_ch (1-4, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 */
	{ GPIOB, GPIO_Pin_7, NONE, TIM4, TIM_Channel_2, NONE },
	{ GPIOB, GPIO_Pin_6, NONE, TIM4, TIM_Channel_1, NONE },
	{ GPIOB, GPIO_Pin_5, NONE, NULL, NONE, NONE },
	{ GPIOB, GPIO_Pin_4, NONE, NULL, NONE, NONE },
	{ GPIOB, GPIO_Pin_3, NONE, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_15, NONE, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_14, NONE, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_13, NONE, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_8, NONE, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_9, NONE, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_0, ADC_Channel_0, TIM2, TIM_Channel_1, NONE },
	{ GPIOA, GPIO_Pin_1, ADC_Channel_1, TIM2, TIM_Channel_2, NONE },
	{ GPIOA, GPIO_Pin_4, ADC_Channel_4, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_5, ADC_Channel_5, NULL, NONE, NONE },
	{ GPIOA, GPIO_Pin_6, ADC_Channel_6, TIM3, TIM_Channel_1, NONE },
	{ GPIOA, GPIO_Pin_7, ADC_Channel_7, TIM3, TIM_Channel_2, NONE },
	{ GPIOB, GPIO_Pin_0, ADC_Channel_8, TIM3, TIM_Channel_3, NONE },
	{ GPIOB, GPIO_Pin_1, ADC_Channel_9, TIM3, TIM_Channel_4, NONE },
	{ GPIOA, GPIO_Pin_3, ADC_Channel_3, TIM2, TIM_Channel_4, NONE },
	{ GPIOA, GPIO_Pin_2, ADC_Channel_2, TIM2, TIM_Channel_3, NONE },
	{ GPIOA, GPIO_Pin_10, NONE, NULL, NONE, NONE }
};

/*
 * Basic variables
 */

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP, or INPUT_PULLDOWN
 */
void pinMode(uint16_t pin, PinMode setMode) {

	if (setMode == NONE )
		return;

	GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
	uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;

	GPIO_InitTypeDef GPIO_InitStructure;

	if (gpio_port == GPIOA ) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	} else if (gpio_port == GPIOB ) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}

	GPIO_InitStructure.GPIO_Pin = gpio_pin;

	switch (setMode) {

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

	default:
		break;
	}

	GPIO_Init(gpio_port, &GPIO_InitStructure);
}

/*
 * @brief Initialize the ADC peripheral.
 */
void Adc_Init() {
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	// ADCCLK = PCLK2/4
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);

	// Enable ADC1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//Work in Progress
	//To Do
/*
	// Enable GPIO clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	uint8_t pin = FIRST_ANALOG_PIN;
	uint8_t rank = 1;	//FIRST_ANALOG_PIN's rank

	// Configure ADC Channels as analog input
	for(pin = FIRST_ANALOG_PIN; pin < (FIRST_ANALOG_PIN + TOTAL_ANALOG_PINS); pin++)
	{
		GPIO_InitStructure.GPIO_Pin = PIN_MAP[pin].gpio_pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
		GPIO_Init(PIN_MAP[pin].gpio_peripheral, &GPIO_InitStructure);
	}
*/

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = TOTAL_ANALOG_PINS;

	ADC_Init(ADC1, &ADC_InitStructure);

	//Work in Progress
	//To Do
/*
	// ADC1 regular channels configurations
	for(pin = FIRST_ANALOG_PIN; pin < (FIRST_ANALOG_PIN + TOTAL_ANALOG_PINS); pin++, rank++)
	{
		ADC_RegularChannelConfig(ADC1, PIN_MAP[pin].adc_channel, rank, ADC_SAMPLING_TIME);
	}

	// Enable ADC1
	ADC_Cmd(ADC1, ENABLE);

	// Enable ADC1 reset calibaration register
	ADC_ResetCalibration(ADC1);

	// Check the end of ADC1 reset calibration register
	while(ADC_GetResetCalibrationStatus(ADC1));

	// Start ADC1 calibaration
	ADC_StartCalibration(ADC1);

	// Check the end of ADC1 calibration
	while(ADC_GetCalibrationStatus(ADC1));
*/
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void digitalWrite(uint16_t pin, uint8_t value) {
	if (pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode != OUTPUT) {
		return;
	}

	if (value == HIGH) {
		PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin;
	} else if (value == LOW) {
		PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin;
	}
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t digitalRead(uint16_t pin) {
	if (pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode == OUTPUT || PIN_MAP[pin].pin_mode == NONE)
	{
		return -1;
	}

	return GPIO_ReadInputDataBit(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin);
}

/*
 * @brief Read the analog value of a pin. Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 */
int32_t analogRead(uint16_t pin) {
	// Allow people to use 0-7 to define analog pins by checking to see if the values are too low.
	if (pin < FIRST_ANALOG_PIN) {
		pin = pin + FIRST_ANALOG_PIN;
	}

	if (pin >= TOTAL_PINS || PIN_MAP[pin].adc_channel == NONE ) {
		return -1;
	}

	if (adcFirstTime == true) {
		Adc_Init();
		adcFirstTime = false;
	}

	//Work in Progress
	//To Do
/*
	//Start ADC1 Software Conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	return ADC_GetConversionValue(ADC1);
*/
	return -1;
}

/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 */
void analogWrite(uint16_t pin, uint8_t value) {

	if (pin >= TOTAL_PINS || PIN_MAP[pin].timer_peripheral == NULL ) {
		return;
	}

	// TODO: Implement
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
uint32_t millis() {
	// TODO: Implement this.
	return -1;
}

/*
 * @brief This should block for a certain number of milliseconds.
 *        There are a number of ways to implement this, but I borrowed the one that Wiring/Arduino uses;
 *        Using the millis() function to check if a certain amount of time has passed.
 */
void delay(uint32_t ms) {
	//uint32_t start = millis();
	//while(millis() - start < ms);
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
void delayMicroseconds(uint32_t us) {
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
