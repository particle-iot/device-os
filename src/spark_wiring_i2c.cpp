/**
 ******************************************************************************
 * @file    spark_wiring_i2c.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

#include "spark_wiring_i2c.h"

// Initialize Class Variables //////////////////////////////////////////////////
I2C_InitTypeDef TwoWire::I2C_InitStructure;

bool TwoWire::I2C_SetAsSlave = false;
bool TwoWire::I2C_Enabled = false;

uint8_t TwoWire::rxBuffer[BUFFER_LENGTH];
uint8_t TwoWire::rxBufferIndex = 0;
uint8_t TwoWire::rxBufferLength = 0;

uint8_t TwoWire::txAddress = 0;
uint8_t TwoWire::txBuffer[BUFFER_LENGTH];
uint8_t TwoWire::txBufferIndex = 0;
uint8_t TwoWire::txBufferLength = 0;

uint8_t TwoWire::transmitting = 0;
void (*TwoWire::user_onRequest)(void);
void (*TwoWire::user_onReceive)(int);

//#define BufferSize           	32
//#define I2C1_SLAVE_ADDRESS7     0x00
//#define Transmitter             0x00
//#define Receiver                0x01

//uint8_t I2C1_Buffer_Tx[BufferSize];
//uint8_t I2C1_Buffer_Rx[BufferSize];
//__IO uint8_t Tx_Idx = 0, Rx_Idx = 0;
//__IO uint8_t Direction = Transmitter;

// Constructors ////////////////////////////////////////////////////////////////

TwoWire::TwoWire()
{
}

// Public Methods //////////////////////////////////////////////////////////////

void TwoWire::begin(void)
{
	rxBufferIndex = 0;
	rxBufferLength = 0;

	txBufferIndex = 0;
	txBufferLength = 0;

	//NVIC_InitTypeDef  NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	pinMode(SCL, AF_OUTPUT_DRAIN);
	pinMode(SDA, AF_OUTPUT_DRAIN);

	I2C_DeInit(I2C1);

	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	if(I2C_SetAsSlave != true)
	{
		I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	}
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;
	I2C_Init(I2C1, &I2C_InitStructure);

	//	I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
	//
	//	NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
	//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
	//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//	NVIC_Init(&NVIC_InitStructure);

	I2C_Cmd(I2C1, ENABLE);

	I2C_Enabled = true;
}

void TwoWire::begin(uint8_t address)
{
//	I2C_SetAsSlave = true;
//
//	if(I2C_Enabled != false)
//	{
//		I2C_Cmd(I2C1, DISABLE);
//	}
//
//	//attachSlaveTxEvent(onRequestService);
//	//attachSlaveRxEvent(onReceiveService);
//	I2C_InitStructure.I2C_OwnAddress1 = address;
//
//	begin();
}

void TwoWire::begin(int address)
{
//	begin((uint8_t)address);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
	uint32_t _millis;

	// clamp to buffer length
	if(quantity > BUFFER_LENGTH){
		quantity = BUFFER_LENGTH;
	}

	/* Send START condition */
	I2C_GenerateSTART(I2C1, ENABLE);

	_millis = millis();
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
	{
		if(EVENT_TIMEOUT < (millis() - _millis)) return 0;
	}

	/* Send Slave address for read */
	I2C_Send7bitAddress(I2C1, address, I2C_Direction_Receiver);

	_millis = millis();
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
	{
		if(EVENT_TIMEOUT < (millis() - _millis)) return 0;
	}

	/* perform blocking read into buffer */
	uint8_t *pBuffer = rxBuffer;
	uint8_t numByteToRead = quantity;
	uint8_t bytesRead = 0;

	/* While there is data to be read */
	while(numByteToRead)
	{
		if(numByteToRead == 1 && sendStop == true)
		{
			/* Disable Acknowledgement */
			I2C_AcknowledgeConfig(I2C1, DISABLE);

			/* Send STOP Condition */
			I2C_GenerateSTOP(I2C1, ENABLE);
		}

		if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
		{
			/* Read a byte from the Slave */
			*pBuffer = I2C_ReceiveData(I2C1);

			bytesRead++;

			/* Point to the next location where the byte read will be saved */
			pBuffer++;

			/* Decrement the read bytes counter */
			numByteToRead--;
		}
	}

	/* Enable Acknowledgement to be ready for another reception */
	I2C_AcknowledgeConfig(I2C1, ENABLE);

	// set rx buffer iterator vars
	rxBufferIndex = 0;
	rxBufferLength = bytesRead;

	return bytesRead;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
	return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
	return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop)
{
	return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
}

void TwoWire::beginTransmission(uint8_t address)
{
	// indicate that we are transmitting
	transmitting = 1;
	// set address of targeted slave
	txAddress = address;
	// reset tx buffer iterator vars
	txBufferIndex = 0;
	txBufferLength = 0;
}

void TwoWire::beginTransmission(int address)
{
	beginTransmission((uint8_t)address);
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to 
//	perform a repeated start. 
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(uint8_t sendStop)
{
	uint32_t _millis;

	// transmit buffer (blocking)

	/* Send START condition */
	I2C_GenerateSTART(I2C1, ENABLE);

	_millis = millis();
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
	{
		if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
	}

	/* Send Slave address for write */
	I2C_Send7bitAddress(I2C1, txAddress, I2C_Direction_Transmitter);

	_millis = millis();
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
	}

	uint8_t *pBuffer = txBuffer;
	uint8_t NumByteToWrite = txBufferLength;

	/* While there is data to be written */
	while(NumByteToWrite--)
	{
		/* Send the current byte to slave */
		I2C_SendData(I2C1, *pBuffer);

		/* Point to the next byte to be written */
		pBuffer++;

		_millis = millis();
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		{
			if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
		}
	}

	if(sendStop == true)
	{
		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
	}

	// reset tx buffer iterator vars
	txBufferIndex = 0;
	txBufferLength = 0;

	// indicate that we are done transmitting
	transmitting = 0;

	return 0;
}

//	This provides backwards compatibility with the original
//	definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire::endTransmission(void)
{
	return endTransmission(true);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(uint8_t data)
{
	if(transmitting){
		// in master transmitter mode
		// don't bother if buffer is full
		if(txBufferLength >= BUFFER_LENGTH){
			setWriteError();
			return 0;
		}
		// put byte in tx buffer
		txBuffer[txBufferIndex] = data;
		++txBufferIndex;
		// update amount in buffer
		txBufferLength = txBufferIndex;
	}else{
//		// in slave send mode
//		// reply to master
//		write(&data, 1);
	}
	return 1;
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
	if(transmitting){
		// in master transmitter mode
		for(size_t i = 0; i < quantity; ++i){
			write(data[i]);
		}
	}else{
//		// in slave send mode
//		// reply to master
//		while(!I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED));
//
//		uint8_t *pBuffer = txBuffer;
//		uint8_t NumByteToWrite = quantity;
//
//		/* While there is data to be written */
//		while(NumByteToWrite--)
//		{
//			/* Send the current byte to master */
//			I2C_SendData(I2C1, *pBuffer);
//
//			/* Point to the next byte to be written */
//			pBuffer++;
//
//			while (!I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_BYTE_TRANSMITTED));
//		}
	}
	return quantity;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::available(void)
{
	return rxBufferLength - rxBufferIndex;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::read(void)
{
	int value = -1;

	// get each successive byte on each call
	if(rxBufferIndex < rxBufferLength){
		value = rxBuffer[rxBufferIndex];
		++rxBufferIndex;
	}

	return value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::peek(void)
{
	int value = -1;

	if(rxBufferIndex < rxBufferLength){
		value = rxBuffer[rxBufferIndex];
	}

	return value;
}

void TwoWire::flush(void)
{
	// XXX: to be implemented.
}

// behind the scenes function that is called when data is received
void TwoWire::onReceiveService(uint8_t* inBytes, int numBytes)
{
	// don't bother if user hasn't registered a callback
	if(!user_onReceive){
		return;
	}
	// don't bother if rx buffer is in use by a master requestFrom() op
	// i know this drops data, but it allows for slight stupidity
	// meaning, they may not have read all the master requestFrom() data yet
	if(rxBufferIndex < rxBufferLength){
		return;
	}
	// copy twi rx buffer into local read buffer
	// this enables new reads to happen in parallel
	for(uint8_t i = 0; i < numBytes; ++i){
		rxBuffer[i] = inBytes[i];
	}
	// set rx iterator vars
	rxBufferIndex = 0;
	rxBufferLength = numBytes;
	// alert user program
	user_onReceive(numBytes);
}

// behind the scenes function that is called when data is requested
void TwoWire::onRequestService(void)
{
	// don't bother if user hasn't registered a callback
	if(!user_onRequest){
		return;
	}
	// reset tx buffer iterator vars
	// !!! this will kill any pending pre-master sendTo() activity
	txBufferIndex = 0;
	txBufferLength = 0;
	// alert user program
	user_onRequest();
}

// sets function called on slave write
void TwoWire::onReceive( void (*function)(int) )
{
	user_onReceive = function;
}

// sets function called on slave read
void TwoWire::onRequest( void (*function)(void) )
{
	user_onRequest = function;
}

/*******************************************************************************
 * Function Name  : Wiring_I2C1_EV_Interrupt_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Event interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Wiring_I2C1_EV_Interrupt_Handler(void)
{
//	switch (I2C_GetLastEvent(I2C1))
//	{
//	case I2C_EVENT_MASTER_MODE_SELECT:                 /* EV5 */
//		if(Direction == Transmitter)
//		{
//			/* Master Transmitter ----------------------------------------------*/
//			/* Send slave Address for write */
//			I2C_Send7bitAddress(I2C1, I2C1_SLAVE_ADDRESS7, I2C_Direction_Transmitter);
//		}
//		else
//		{
//			/* Master Receiver -------------------------------------------------*/
//			/* Send slave Address for read */
//			I2C_Send7bitAddress(I2C1, I2C1_SLAVE_ADDRESS7, I2C_Direction_Receiver);
//
//		}
//		break;
//
//		/* Master Transmitter --------------------------------------------------*/
//		/* Test on I2C1 EV6 and first EV8 and clear them */
//	case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
//
//		/* Send the first data */
//		I2C_SendData(I2C1, I2C1_Buffer_Tx[Tx_Idx++]);
//		break;
//
//		/* Test on I2C1 EV8 and clear it */
//	case I2C_EVENT_MASTER_BYTE_TRANSMITTING:  /* Without BTF, EV8 */
//		if(Tx_Idx < (BufferSize))
//		{
//			/* Transmit I2C1 data */
//			I2C_SendData(I2C1, I2C1_Buffer_Tx[Tx_Idx++]);
//
//		}
//		else
//		{
//			I2C_TransmitPEC(I2C1, ENABLE);
//			I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
//		}
//		break;
//
//	case I2C_EVENT_MASTER_BYTE_TRANSMITTED: /* With BTF EV8-2 */
//		I2C_ITConfig(I2C1, I2C_IT_BUF, ENABLE);
//		/* I2C1 Re-START Condition */
//		I2C_GenerateSTART(I2C1, ENABLE);
//		break;
//
//		/* Master Receiver -------------------------------------------------------*/
//	case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:
//		if(BufferSize == 1)
//		{
//			/* Disable I2C1 acknowledgement */
//			I2C_AcknowledgeConfig(I2C1, DISABLE);
//			/* Send I2C1 STOP Condition */
//			I2C_GenerateSTOP(I2C1, ENABLE);
//		}
//		break;
//
//		/* Test on I2C1 EV7 and clear it */
//	case I2C_EVENT_MASTER_BYTE_RECEIVED:
//		/* Store I2C1 received data */
//		I2C1_Buffer_Rx[Rx_Idx++] = I2C_ReceiveData (I2C1);
//		/* Disable ACK and send I2C1 STOP condition before receiving the last data */
//		if(Rx_Idx == (BufferSize - 1))
//		{
//			/* Disable I2C1 acknowledgement */
//			I2C_AcknowledgeConfig(I2C1, DISABLE);
//			/* Send I2C1 STOP Condition */
//			I2C_GenerateSTOP(I2C1, ENABLE);
//		}
//		break;
//
//		/* Slave Transmitter ---------------------------------------------------*/
//	case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:  /* EV1 */
//
//		/* Transmit I2C1 data */
//		I2C_SendData(I2C1, I2C1_Buffer_Tx[Tx_Idx++]);
//		break;
//
//	case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:             /* EV3 */
//		/* Transmit I2C1 data */
//		I2C_SendData(I2C1, I2C1_Buffer_Tx[Tx_Idx++]);
//		break;
//
//
//		/* Slave Receiver ------------------------------------------------------*/
//	case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:     /* EV1 */
//		break;
//
//	case I2C_EVENT_SLAVE_BYTE_RECEIVED:                /* EV2 */
//		/* Store I2C1 received data */
//		I2C1_Buffer_Rx[Rx_Idx++] = I2C_ReceiveData(I2C1);
//
//		if(Rx_Idx == BufferSize)
//		{
//			I2C_TransmitPEC(I2C1, ENABLE);
//			Direction = Receiver;
//		}
//		break;
//
//	case I2C_EVENT_SLAVE_STOP_DETECTED:                /* EV4 */
//		/* Clear I2C1 STOPF flag: read of I2C_SR1 followed by a write on I2C_CR1 */
//		(void)(I2C_GetITStatus(I2C1, I2C_IT_STOPF));
//		I2C_Cmd(I2C1, ENABLE);
//		break;
//
//	default:
//		break;
//	}
}

/*******************************************************************************
 * Function Name  : Wiring_I2C1_ER_Interrupt_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Error interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Wiring_I2C1_ER_Interrupt_Handler(void)
{
	//To Do
}

bool TwoWire::isEnabled() {
	return I2C_Enabled;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

TwoWire Wire = TwoWire();

