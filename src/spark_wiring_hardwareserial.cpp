#include "spark_wiring_hardwareserial.h"

#define SERIAL_BUFFER_SIZE 64

struct ring_buffer
{
  unsigned char buffer[SERIAL_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
};

ring_buffer rx_buffer  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer  =  { { 0 }, 0, 0 };


//This routine is called by the USART RX ISR
inline void store_char(unsigned char c, ring_buffer *buffer)
{
  unsigned int i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if (i != buffer->tail) {
    buffer->buffer[buffer->head] = c;
    buffer->head = i;
  }
}


//Implement an USART TX ISR here
/*
ISR(USART1_UDRE_vect)
{
  if (tx_buffer1.head == tx_buffer1.tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR1B, UDRIE1);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    unsigned char c = tx_buffer1.buffer[tx_buffer1.tail];
    tx_buffer1.tail = (tx_buffer1.tail + 1) % SERIAL_BUFFER_SIZE;
	
    UDR1 = c;
  }
}
*/

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

	//NVIC module configuration
	NVIC_InitTypeDef NVIC_InitStructure; 
	// Place the vector table into FLASH
	//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0); 
	// Enabling interrupt from USART2
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);

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

	//Enabling interrupt from USART2
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	transmitting = false;

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

	//NVIC module configuration
	NVIC_InitTypeDef NVIC_InitStructure; 
	// Place the vector table into FLASH
	//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0); 
	// Enabling interrupt from USART2
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 14; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);

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

	//Enabling interrupt from USART2
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	//Enable USART2 global interrupt
    NVIC_EnableIRQ(USART2_IRQn);

	transmitting = false;

	//serial1_enabled = true;
}

//todo - make a clean exit
void HardwareSerial::end()
{
	// wait for transmission of outgoing data
  while (_tx_buffer->head != _tx_buffer->tail)
    ;
	// Disable the USART
	USART_Cmd(USART2, DISABLE);
	//serial1_enabled = false;
	// clear any received data
  	_rx_buffer->head = _rx_buffer->tail;

}

//serial1 only
// uint8_t HardwareSerial::available(void)
// {
//   // Check if the USART Receive Data Register is not empty
// 	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
// 		return 1;
// 	else
// 		return 0;
// }

int HardwareSerial::available(void)
{
  return (unsigned int)(SERIAL_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

int HardwareSerial::peek(void)
{
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    return _rx_buffer->buffer[_rx_buffer->tail];
  }
}

int HardwareSerial::read(void)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
    _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
    return c;
  }
}

void HardwareSerial::flush()
{
  // UDR is kept full while the buffer is not empty, so TXC triggers when EMPTY && SENT
  //while (transmitting && ! (*_ucsra & _BV(TXC0)));
  transmitting = false;
}

size_t HardwareSerial::write(uint8_t c)
{
  unsigned int i = (_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE;
	
  // If the output buffer is full, there's nothing for it other than to 
  // wait for the interrupt handler to empty it a bit
  // ???: return 0 here instead?
  while (i == _tx_buffer->tail)
    ;
	
  _tx_buffer->buffer[_tx_buffer->head] = c;
  _tx_buffer->head = i;
	
  //sbi(*_ucsrb, _udrie);
  // clear the TXC bit -- "can be cleared by writing a one to its bit location"
  transmitting = true;
  //sbi(*_ucsra, TXC0);
  
  return 1;
}

HardwareSerial::operator bool() {
	return true;
}


// Preinstantiate Objects //////////////////////////////////////////////////////

HardwareSerial SerialW;