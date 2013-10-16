#include "spark_wiring_hardwareserial.h"

// Public Methods //////////////////////////////////////////////////////////////


//Implementing serial1 only
void HardwareSerial::begin(uint32_t baudRate)
{

	USART_InitTypeDef USART_InitStructure;

	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// Configure USART Rx as input floating
	pinMode(RX, INPUT);

	// Configure USART Tx as alternate function push-pull
	pinMode(TX, AF_OUTPUT_PUSHPULL);

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

	//serial1_enabled = true;

}


//no config done here
//todo - make use of the config parameter
void HardwareSerial::begin(uint32_t baudRate, uint8_t config)
{

	USART_InitTypeDef USART_InitStructure;

	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// Configure USART Rx as input floating
	pinMode(RX, INPUT);

	// Configure USART Tx as alternate function push-pull
	pinMode(TX, AF_OUTPUT_PUSHPULL);

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

	//serial1_enabled = true;
}

//todo - make a clean exit
void HardwareSerial::end()
{
	// Disable the USART
	USART_Cmd(USART2, DISABLE);
	//serial1_enabled = false;

}

//serial1 only
uint8_t HardwareSerial::available(void)
{
  // Check if the USART Receive Data Register is not empty
	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
		return 1;
	else
		return 0;
}
