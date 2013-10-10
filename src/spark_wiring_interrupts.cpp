#include "spark_wiring_interrupts.h"

extern STM32_Pin_Info PIN_MAP[TOTAL_PINS];


//Interrupts
 const uint8_t GPIO_IRQn[] = {
  EXTI0_IRQn,     //0
  EXTI1_IRQn,     //1
  EXTI2_IRQn,     //2
  EXTI3_IRQn,     //3
  EXTI4_IRQn,     //4
  EXTI9_5_IRQn,   //5
  EXTI9_5_IRQn,   //6
  EXTI9_5_IRQn,   //7
  EXTI9_5_IRQn,   //8
  EXTI9_5_IRQn,   //9
  EXTI15_10_IRQn, //10
  EXTI15_10_IRQn, //11
  EXTI15_10_IRQn, //12
  EXTI15_10_IRQn, //13
  EXTI15_10_IRQn, //14
  EXTI15_10_IRQn  //15
 };

// Create a structure for user ISR function pointers
 typedef struct exti_channel {
    void (*handler)();
} exti_channel;

//Array to hold user ISR function pointers
static exti_channel exti_channels[] = {
    { .handler = NULL },  // EXTI0
    { .handler = NULL },  // EXTI1
    { .handler = NULL },  // EXTI2
    { .handler = NULL },  // EXTI3
    { .handler = NULL },  // EXTI4
    { .handler = NULL },  // EXTI5
    { .handler = NULL },  // EXTI6
    { .handler = NULL },  // EXTI7
    { .handler = NULL },  // EXTI8
    { .handler = NULL },  // EXTI9
    { .handler = NULL },  // EXTI10
    { .handler = NULL },  // EXTI11
    { .handler = NULL },  // EXTI12
    { .handler = NULL },  // EXTI13
    { .handler = NULL },  // EXTI14
    { .handler = NULL }  // EXTI15
};

						 
/*******************************************************************************
* Function Name  : attachInterrupt
* Description    : Arduino compatible function to attach hardware interrupts to 
						 the Core pins
* Input          : pin number, user function name and interrupt mode
* Output         : None.
* Return         : None.
*******************************************************************************/

void attachInterrupt(uint16_t pin, voidFuncPtr handler, InterruptMode mode)
{
	uint8_t GPIO_PortSource = 0;	//variable to hold the port number
	uint8_t GPIO_PinSource = 0;	//variable to hold the pin number
	uint8_t PinNumber;				//temp variable to calculate the pin number


	//EXTI structure to init EXT
	EXTI_InitTypeDef EXTI_InitStructure;
	//NVIC structure to set up NVIC controller
	NVIC_InitTypeDef NVIC_InitStructure;

	//Map the Spark pin to the appropriate port and pin on the STM32
	GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
	uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;

	//Select the port source
	if (gpio_port == GPIOA )
	{
		GPIO_PortSource = 0;
	}
	else if (gpio_port == GPIOB )
	{
		GPIO_PortSource = 1;
	}

	//Find out the pin number from the mask
	PinNumber = gpio_pin;
	PinNumber = PinNumber >> 1;
	while(PinNumber)
	{
		PinNumber = PinNumber >> 1;
		GPIO_PinSource++;
	}

	// Register the handler for the user function name
    exti_channels[GPIO_PinSource].handler = handler;

	//Connect EXTI Line to appropriate Pin
	GPIO_EXTILineConfig(GPIO_PortSource, GPIO_PinSource);

	//Configure GPIO EXTI line
	EXTI_InitStructure.EXTI_Line = gpio_pin;//EXTI_Line;
	
	//select the interrupt mode
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	switch (mode)
	{
		//case LOW:
			//There is no LOW mode in STM32, so using falling edge as default
			//EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
			//break;
		case CHANGE:
			//generate interrupt on rising or falling edge
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
			break;
		case RISING:
			//generate interrupt on rising edge
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
			break;
		case FALLING:
			//generate interrupt on falling edge
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
			break;
	}

	//enable EXTI line
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	//send values to registers
	EXTI_Init(&EXTI_InitStructure);

	//configure NVIC
	//select NVIC channel to configure
	NVIC_InitStructure.NVIC_IRQChannel = GPIO_IRQn[GPIO_PinSource];
	//set priority to lowest
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	//set subpriority to lowest
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	//enable IRQ channel
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//update NVIC registers
	NVIC_Init(&NVIC_InitStructure);
}


/*******************************************************************************
* Function Name  : detachInterrupt
* Description    : Arduino compatible function to detach hardware interrupts that
						 were asssigned previously using attachInterrupt
* Input          : pin number to which the interrupt was attached
* Output         : None.
* Return         : None.
*******************************************************************************/
void detachInterrupt(uint16_t pin)
{
	uint8_t GPIO_PinSource = 0;	//variable to hold the pin number
	uint8_t PinNumber;				//temp variable to calculate the pin number

	//Map the Spark Core pin to the appropriate pin on the STM32
	uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;

	//Clear the pending interrupt flag for that interrupt pin
	EXTI_ClearITPendingBit(gpio_pin);

	//EXTI structure to init EXT
	EXTI_InitTypeDef EXTI_InitStructure;

	//Find out the pin number from the mask
	PinNumber = gpio_pin;
	PinNumber = PinNumber >> 1;
	while(PinNumber)
	{
		PinNumber = PinNumber >> 1;
		GPIO_PinSource++;
	}

	//Select the appropriate EXTI line
	EXTI_InitStructure.EXTI_Line = gpio_pin;
	//disable that EXTI line
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
	//send values to registers
	EXTI_Init(&EXTI_InitStructure);

	//unregister the user's handler
	exti_channels[GPIO_PinSource].handler = NULL;

}

/*******************************************************************************
* Function Name  : noInterrupts
* Description    : Disable all external interrupts
* Output         : None.
* Return         : None.
*******************************************************************************/
void noInterrupts(void)
{
	//Only disable the interrupts that are exposed to the user
	NVIC_DisableIRQ(EXTI0_IRQn);
	NVIC_DisableIRQ(EXTI1_IRQn);
	NVIC_DisableIRQ(EXTI3_IRQn);
	NVIC_DisableIRQ(EXTI4_IRQn);
	NVIC_DisableIRQ(EXTI9_5_IRQn);
}


/*******************************************************************************
* Function Name  : interrupts
* Description    : Enable all external interrupts
* Output         : None.
* Return         : None.
*******************************************************************************/
void interrupts(void)
{
	//Only enable the interrupts that are exposed to the user
	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);

	
}

//Interrupt Handlers
//Inspired from the implepentation of interrupts in libmaple
/*******************************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : Each interrupt has a unique ISR function that is called. 
						 We use this to call the user ISR assigned in the 
						 attachInterrupt function.
						 This function is called when EXTIO is asserted.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EXTI0_IRQHandler(void)
{
	//Check if EXTI_Line0 is asserted
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
    	//call user function here
    	userISRFunction_single(0);
    }
    //we need to clear line pending bit manually
    EXTI_ClearITPendingBit(EXTI_Line0);

}

/*******************************************************************************
* Function Name  : EXTI1_IRQHandler
* Description    : Each interrupt has a unique ISR function that is called. 
						 We use this to call the user ISR assigned in the 
						 attachInterrupt function.
						 This function is called when EXTI1 is asserted.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EXTI1_IRQHandler(void)
{
	//Check if EXTI_Line1 is asserted
    if(EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
    	//call user function here
    	userISRFunction_single(1);
    }
    //we need to clear line pending bit manually
    EXTI_ClearITPendingBit(EXTI_Line1);
	
}


/* USED BY MODE BUTTON - not going to implement this for the time being
void EXTI2_IRQHandler(void)
{	
}
*/


/*******************************************************************************
* Function Name  : EXTI3_IRQHandler
* Description    : Each interrupt has a unique ISR function that is called. 
						 We use this to call the user ISR assigned in the 
						 attachInterrupt function.
						 This function is called when EXTI3 is asserted.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EXTI3_IRQHandler(void)
{
	//Check if EXTI_Line3 is asserted
    if(EXTI_GetITStatus(EXTI_Line3) != RESET)
    {
    	//call user function here
    	userISRFunction_single(3);

    }
    //we need to clear line pending bit manually
    EXTI_ClearITPendingBit(EXTI_Line3);
	
}

/*******************************************************************************
* Function Name  : EXTI4_IRQHandler
* Description    : Each interrupt has a unique ISR function that is called. 
						 We use this to call the user ISR assigned in the 
						 attachInterrupt function.
						 This function is called when EXTI4 is asserted.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EXTI4_IRQHandler(void)
{
	//Check if EXTI_Line4 is asserted
    if(EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
    	//call user function here
    	userISRFunction_single(4);
    }
    //we need to clear line pending bit manually
    EXTI_ClearITPendingBit(EXTI_Line4);
	
}

/*******************************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : Each interrupt has a unique ISR function that is called. 
						 We use this to call the user ISR assigned in the 
						 attachInterrupt function.
						 This function is called when EXTI 5 through 9 are asserted.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EXTI9_5_IRQHandler(void)
{
	userISRFunction_multiple(5, 9);
}


/* USED BY CC3000 and SPI - - not going to implement this here
Its currently being implemented in stm32_it.c
//EXTI 10 to 15
void EXTI15_10_IRQHandler(void)
{	
}
*/


/*******************************************************************************
* Function Name  : userISRFunction_single
* Description    : This function is called by any of the interrupt handlers. It 
						 essentially fetches the user function pointer from the array
						 and calls it.
* Input          : EXTI - Interrupt number - 
* Output         : None.
* Return         : None.
*******************************************************************************/
void userISRFunction_single(uint8_t intNumber)
{
	//fetch the user function pointer from the array
	voidFuncPtr userISR_Handle = exti_channels[intNumber].handler;
	//Check to see if the user handle is NULL
	if (!userISR_Handle) 
 	{
         return;
    }
  //This is the call to the actual user ISR function
     userISR_Handle();
}


/*******************************************************************************
* Function Name  : userISRFunction_multiple
* Description    : This function is similar to userISRFunction_single() but it 
						 handles and sorts through multiple interrupts that are associated
						 with the same handle like EXTI 5 to 9 and EXTI 10 to 15.
* Input          : EXTI - Interrupt number range - 
* Output         : None.
* Return         : None.
*******************************************************************************/
void userISRFunction_multiple(uint8_t intNumStart, uint8_t intNUmEnd)
{
	uint8_t count =0;
	uint32_t EXTI_LineMask = 0;

	//Go through all the interrupt flags and see which ones were asserted
	for (count = intNumStart; count <= intNUmEnd; count++)
	{
		//Create a mask from the pin number
		EXTI_LineMask = (1U << count);
		//Check if the interrupt was asserted
		if(EXTI_GetITStatus(EXTI_LineMask) != RESET)
		{
			//Fetch the user function from the array of user function pointers
			voidFuncPtr userISR_Handle = exti_channels[count].handler;
			//Check to see if the user handle is NULL
			if (userISR_Handle) 
			{
				//This is the actual call to the user ISR function
				userISR_Handle();
				//Clear the appropriate interrupt flag before exiting
				EXTI_ClearITPendingBit(EXTI_LineMask);
			}
		}
	}
}
