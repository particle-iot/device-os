/*
  TWI/I2C library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#include "spark_wiring_i2c.h"

// Initialize Class Variables //////////////////////////////////////////////////
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

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	pinMode(SCL, AF_OUTPUT_DRAIN);
	pinMode(SDA, AF_OUTPUT_DRAIN);

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

	//I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE);

	I2C_Cmd(I2C1, ENABLE);

	I2C_Enabled = true;
}

void TwoWire::begin(uint8_t address)
{
	I2C_SetAsSlave = true;

	if(I2C_Enabled != false)
	{
		I2C_Cmd(I2C1, DISABLE);
	}

	//twi_attachSlaveTxEvent(onRequestService);
	//twi_attachSlaveRxEvent(onReceiveService);
	I2C_InitStructure.I2C_OwnAddress1 = address;

	begin();
}

void TwoWire::begin(int address)
{
	begin((uint8_t)address);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
	// clamp to buffer length
	if(quantity > BUFFER_LENGTH){
		quantity = BUFFER_LENGTH;
	}

	/* Send START condition */
	I2C_GenerateSTART(I2C1, ENABLE);

	/* Test on EV5 and clear it */
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	/* Send EEPROM address for read */
	I2C_Send7bitAddress(I2C1, address, I2C_Direction_Receiver);

	/* Test on EV6 and clear it */
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

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

		/* Test on EV7 and clear it */
		if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
		{
			/* Read a byte from the EEPROM */
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
	// transmit buffer (blocking)

	/* Send START condition */
	I2C_GenerateSTART(I2C1, ENABLE);

	/* Test on EV5 and clear it */
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	/* Send EEPROM address for write */
	I2C_Send7bitAddress(I2C1, txAddress, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	uint8_t *pBuffer = txBuffer;
	uint8_t NumByteToWrite = txBufferLength;

	/* While there is data to be written */
	while(NumByteToWrite--)
	{
		/* Send the current byte */
		I2C_SendData(I2C1, *pBuffer);

		/* Point to the next byte to be written */
		pBuffer++;

		/* Test on EV8 and clear it */
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
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
		// in slave send mode
		// reply to master
		//Slave: transmit(&data, 1) => //To Do
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
		// in slave send mode
		// reply to master
		// Slave: transmit(data, quantity) => To Do
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

// Preinstantiate Objects //////////////////////////////////////////////////////

TwoWire Wire = TwoWire();

