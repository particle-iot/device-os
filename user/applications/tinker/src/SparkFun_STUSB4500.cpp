/*
  This is a library written for the STUSB4500 Power Delivery Board.
  SparkFun sells these at its website: https://www.sparkfun.com

  The functions were based on the NVM_Flasher code for
  the STUSB4500 which can be found here: https://github.com/usb-c/STUSB4500

  Written by Alex Wende @ SparkFun Electronics, February 6th, 2020

  https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.6

  For licence information see LICENSE.md
  https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library/blob/master/LICENSE.md
*/

#include "SparkFun_STUSB4500.h"

uint8_t sector[5][8];
uint8_t readSectors = 0;

uint8_t STUSB4500::begin(uint8_t deviceAddress, TwoWire &wirePort)
{
  readSectors = 0;
  _deviceAddress = deviceAddress; //If provided, store the I2C address from user
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _i2cPort->beginTransmission(_deviceAddress);

  uint8_t error = _i2cPort->endTransmission();

  if(error == 0)
  {
	if(readSectors == 0)
    {
      read();
	  readSectors = 1;
    }
	return true; //Device online!
  }
  else return false;          //Device not attached?
}

void STUSB4500::read(void)
{
  uint8_t Buffer[1];
  readSectors = 1;
  //Read Current Parameters
  //-Enter Read Mode
  //-Read Sector[x][-]
  //---------------------------------
  //Enter Read Mode
  Buffer[0]=FTP_CUST_PASSWORD;  /* Set Password 0x95->0x47*/
  I2C_Write_USB_PD(FTP_CUST_PASSWORD_REG,Buffer,1);

  Buffer[0]= 0; /* NVM internal controller reset 0x96->0x00*/
  I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1);

  Buffer[0]= FTP_CUST_PWR | FTP_CUST_RST_N; /* Set PWR and RST_N bits 0x96->0xC0*/
  I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1);

  //--- End of CUST_EnterReadMode

  for(uint8_t i=0;i<5;i++)
  {
    Buffer[0]= FTP_CUST_PWR | FTP_CUST_RST_N; /* Set PWR and RST_N bits 0x96->0xC0*/
    I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1);

    Buffer[0]= (READ & FTP_CUST_OPCODE);  /* Set Read Sectors Opcode 0x97->0x00*/
    I2C_Write_USB_PD(FTP_CTRL_1,Buffer,1);

    Buffer[0]= (i & FTP_CUST_SECT) |FTP_CUST_PWR |FTP_CUST_RST_N | FTP_CUST_REQ;
    I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1);  /* Load Read Sectors Opcode */

    do
    {
      I2C_Read_USB_PD(FTP_CTRL_0,Buffer,1); /* Wait for execution */
    }
    while(Buffer[0] & FTP_CUST_REQ); //The FTP_CUST_REQ is cleared by NVM controller when the operation is finished.

    I2C_Read_USB_PD(RW_BUFFER,&sector[i][0],8);
  }

  CUST_ExitTestMode();

  // NVM settings get loaded into the volatile registers after a hard reset or power cycle.
  // Below we will copy over some of the saved NVM settings to the I2C registers
  uint8_t currentValue;

  //PDO Number
  setPdoNumber((sector[3][2] & 0x06)>>1);


  //PDO1 - fixed at 5V and is unable to change
  setVoltage(1,5.0);

  currentValue = (sector[3][2]&0xF0) >> 4;
  if(currentValue == 0)      setCurrent(1,0);
  else if(currentValue < 11) setCurrent(1,currentValue * 0.25 + 0.25);
  else                       setCurrent(1,currentValue * 0.50 - 2.50);

  //PDO2
  setVoltage(2,((sector[4][1]<<2) + (sector[4][0]>>6))/20.0);

  currentValue = (sector[3][4]&0x0F);
  if(currentValue == 0)      setCurrent(2,0);
  else if(currentValue < 11) setCurrent(2,currentValue * 0.25 + 0.25);
  else                       setCurrent(2,currentValue * 0.50 - 2.50);

  //PDO3
  setVoltage(3,(((sector[4][3]&0x03)<<8) + sector[4][2])/20.0);

  currentValue = (sector[3][5]&0xF0) >> 4;
  if(currentValue == 0)      setCurrent(3,0);
  else if(currentValue < 11) setCurrent(3,currentValue * 0.25 + 0.25);
  else                       setCurrent(3,currentValue * 0.50 - 2.50);
}

void STUSB4500::write(uint8_t defaultVals)
{
  if(defaultVals == 0)
  {
  	uint8_t nvmCurrent[] = { 0, 0, 0};
  	float voltage[] = { 0, 0, 0};

  	uint32_t digitalVoltage=0;

  	//Load current values into NVM
  	for(byte i=0; i<3; i++)
  	{
  	  uint32_t pdoData = readPDO(i+1);
  	  float current = (pdoData&0x3FF)*0.01; //The current is the first 10-bits of the 32-bit PDO register (10mA resolution)

  	  if(current > 5.0) current = 5.0; //Constrain current value to 5A max

  	  /*Convert current from float to 4-bit value
       -current from 0.5-3.0A is set in 0.25A steps
       -current from 3.0-5.0A is set in 0.50A steps
      */
  	  if(current < 0.5)     nvmCurrent[i] = 0;
  	  else if(current <= 3) nvmCurrent[i] = (4*current)-1;
  	  else                  nvmCurrent[i] = (2*current)+5;


  	  digitalVoltage = (pdoData>>10)&0x3FF; //The voltage is bits 10:19 of the 32-bit PDO register
  	  voltage[i] = digitalVoltage/20.0; //Voltage has 50mV resolution

  	  // Make sure the minimum voltage is between 5-20V
  	  if(voltage[i] < 5.0)       voltage[i] = 5.0;
  	  else if(voltage[i] > 20.0) voltage[i] = 20.0;
  	}

  	// load current for PDO1 (sector 3, byte 2, bits 4:7)
    sector[3][2] &= 0x0F;             //clear bits 4:7
    sector[3][2] |= (nvmCurrent[0]<<4);    //load new amperage for PDO1

    // load current for PDO2 (sector 3, byte 4, bits 0:3)
    sector[3][4] &= 0xF0;             //clear bits 0:3
    sector[3][4] |= nvmCurrent[1];     //load new amperage for PDO2

    // load current for PDO3 (sector 3, byte 5, bits 4:7)
    sector[3][5] &= 0x0F;           //clear bits 4:7
    sector[3][5] |= (nvmCurrent[2]<<4);  //set amperage for PDO3

    // The voltage for PDO1 is 5V and cannot be changed

    // PDO2
	// Load voltage (10-bit)
	// -bit 9:2 - sector 4, byte 1, bits 0:7
	// -bit 0:1 - sector 4, byte 0, bits 6:7
	digitalVoltage = voltage[1] * 20;          //convert votlage to 10-bit value
	sector[4][0] &= 0x3F;                        //clear bits 6:7
	sector[4][0] |= ((digitalVoltage&0x03)<<6);  //load voltage bits 0:1 into bits 6:7
	sector[4][1] = (digitalVoltage>>2);          //load bits 2:9

    // PDO3
    // Load voltage (10-bit)
    // -bit 8:9 - sector 4, byte 3, bits 0:1
    // -bit 0:7 - sector 4, byte 2, bits 0:7
    digitalVoltage = voltage[2] * 20;   //convert voltage to 10-bit value
    sector[4][2] = 0xFF & digitalVoltage; //load bits 0:7
    sector[4][3] &= 0xFC;                 //clear bits 0:1
    sector[4][3] |= (digitalVoltage>>8);  //load bits 8:9


    // Load highest priority PDO number from memory
    uint8_t Buffer[1];
    I2C_Read_USB_PD(DPM_PDO_NUMB, Buffer,1);

    //load PDO number (sector 3, byte 2, bits 2:3) for NVM saving
    sector[3][2] &= 0xF9;
    sector[3][2] |= (Buffer[0]<<1);


	CUST_EnterWriteMode(SECTOR_0 | SECTOR_1  | SECTOR_2 | SECTOR_3  | SECTOR_4 );
    CUST_WriteSector(0,&sector[0][0]);
    CUST_WriteSector(1,&sector[1][0]);
    CUST_WriteSector(2,&sector[2][0]);
    CUST_WriteSector(3,&sector[3][0]);
    CUST_WriteSector(4,&sector[4][0]);
    CUST_ExitTestMode();
  }
  else
  {
	uint8_t default_sector[5][8] =
    {
      {0x00,0x00,0xB0,0xAA,0x00,0x45,0x00,0x00},
      {0x10,0x40,0x9C,0x1C,0xFF,0x01,0x3C,0xDF},
      {0x02,0x40,0x0F,0x00,0x32,0x00,0xFC,0xF1},
      {0x00,0x19,0x56,0xAF,0xF5,0x35,0x5F,0x00},
      {0x00,0x4B,0x90,0x21,0x43,0x00,0x40,0xFB}
    };

	CUST_EnterWriteMode(SECTOR_0 | SECTOR_1  | SECTOR_2 | SECTOR_3  | SECTOR_4 );
    CUST_WriteSector(0,&default_sector[0][0]);
    CUST_WriteSector(1,&default_sector[1][0]);
    CUST_WriteSector(2,&default_sector[2][0]);
    CUST_WriteSector(3,&default_sector[3][0]);
    CUST_WriteSector(4,&default_sector[4][0]);
    CUST_ExitTestMode();
  }

}

float STUSB4500::getVoltage(uint8_t pdo_numb)
{
  float voltage=0;
  uint32_t pdoData = readPDO(pdo_numb);

  pdoData = (pdoData>>10)&0x3FF;
  voltage = pdoData / 20.0;

  return voltage;
}

float STUSB4500::getCurrent(uint8_t pdo_numb)
{
  uint32_t pdoData = readPDO(pdo_numb);

  pdoData &= 0x3FF;

  return pdoData * 0.01;
}

uint8_t STUSB4500::getLowerVoltageLimit(uint8_t pdo_numb)
{
  if(pdo_numb == 1) //PDO1
  {
	return 0;
  }
  else if(pdo_numb == 2) //PDO2
  {
	return (sector[3][4]>>4) + 5;
  }
  else //PDO3
  {
	return (sector[3][6] & 0x0F) + 5;
  }
}

uint8_t STUSB4500::getUpperVoltageLimit(uint8_t pdo_numb)
{
  if(pdo_numb == 1) //PDO1
  {
	return (sector[3][3]>>4) + 5;
  }
  else if(pdo_numb == 2) //PDO2
  {
	return (sector[3][5] & 0x0F) + 5;
  }
  else //PDO3
  {
	return (sector[3][6]>>4) + 5;
  }
}

float STUSB4500::getFlexCurrent(void)
{
  uint16_t digitalValue = ((sector[4][4]&0x0F)<<6) + ((sector[4][3]&0xFC)>>2);
  return digitalValue / 100.0;
}

uint8_t STUSB4500::getPdoNumber(void)
{
  uint8_t Buffer[1];
  I2C_Read_USB_PD(DPM_PDO_NUMB, Buffer, 1);

  return Buffer[0]&0x07;
}

uint8_t STUSB4500::getExternalPower(void)
{
  return (sector[3][2]&0x08)>>3;
}

uint8_t STUSB4500::getUsbCommCapable(void)
{
  return (sector[3][2]&0x01);
}

uint8_t STUSB4500::getConfigOkGpio(void)
{
  return (sector[4][4]&0x60)>>5;
}

uint8_t STUSB4500::getGpioCtrl(void)
{
  return (sector[1][0]&0x30)>>4;
}

uint8_t STUSB4500::getPowerAbove5vOnly(void)
{
  return (sector[4][6]&0x08)>>3;
}

uint8_t STUSB4500::getReqSrcCurrent(void)
{
  return (sector[4][6]&0x10)>>4;
}

void STUSB4500::setVoltage(uint8_t pdo_numb, float voltage)
{
  if(pdo_numb < 1) pdo_numb = 1;
  else if(pdo_numb > 3) pdo_numb = 3;

  //Constrain voltage variable to 5-20V
  if(voltage < 5) voltage = 5;
  else if(voltage > 20) voltage = 20;

  // Load voltage to volatile PDO memory (PDO1 needs to remain at 5V)
  if(pdo_numb == 1) voltage = 5;

  voltage *= 20;

  //Replace voltage from bits 10:19 with new voltage
  uint32_t pdoData = readPDO(pdo_numb);

  pdoData &= ~(0xFFC00);
  pdoData |= (uint32_t(voltage)<<10);

  writePDO(pdo_numb, pdoData);
}

void STUSB4500::setCurrent(uint8_t pdo_numb, float current)
{
  // Load current to volatile PDO memory
  current /= 0.01;

  uint32_t intCurrent = current;
  intCurrent &= 0x3FF;

  uint32_t pdoData = readPDO(pdo_numb);
  pdoData &= ~(0x3FF);
  pdoData |= intCurrent;

  writePDO(pdo_numb, pdoData);
}

void STUSB4500::setLowerVoltageLimit(uint8_t pdo_numb, uint8_t value)
{
  //Constrain value to 5-20%
  if(value < 5) value = 5;
  else if(value > 20) value = 20;

  //UVLO1 fixed

  if(pdo_numb == 2) //UVLO2
  {
    //load UVLO (sector 3, byte 4, bits 4:7)
    sector[3][4] &= 0x0F;             //clear bits 4:7
    sector[3][4] |= (value-5)<<4;  //load new UVLO value
  }
  else if(pdo_numb == 3) //UVLO3
  {
    //load UVLO (sector 3, byte 6, bits 0:3)
    sector[3][6] &= 0xF0;
    sector[3][6] |= (value-5);
  }
}

void STUSB4500::setUpperVoltageLimit(uint8_t pdo_numb, uint8_t value)
{
  //Constrain value to 5-20%
  if(value < 5) value = 5;
  else if(value > 20) value = 20;

  if(pdo_numb == 1) //OVLO1
  {
    //load OVLO (sector 3, byte 3, bits 4:7)
    sector[3][3] &= 0x0F;             //clear bits 4:7
    sector[3][3] |= (value-5)<<4;  //load new OVLO value
  }
  else if(pdo_numb == 2) //OVLO2
  {
    //load OVLO (sector 3, byte 5, bits 0:3)
    sector[3][5] &= 0xF0;             //clear bits 0:3
    sector[3][5] |= (value-5);     //load new OVLO value
  }
  else if(pdo_numb == 3) //OVLO3
  {
    //load OVLO (sector 3, byte 6, bits 4:7)
    sector[3][6] &= 0x0F;
    sector[3][6] |= ((value-5)<<4);
  }
}

void STUSB4500::setFlexCurrent(float value)
{
  //Constrain value to 0-5A
  if(value > 5) value = 5;
  else if(value < 0) value = 0;

  uint16_t flex_val = value*100;

  sector[4][3] &= 0x03;                 //clear bits 2:6
  sector[4][3] |= ((flex_val&0x3F)<<2); //set bits 2:6

  sector[4][4] &= 0xF0;                 //clear bits 0:3
  sector[4][4] |= ((flex_val&0x3C0)>>6);//set bits 0:3
}

void STUSB4500::setPdoNumber(uint8_t value)
{
  uint8_t Buffer[1];
  if(value > 3) value = 3;

  //load PDO number to volatile memory
  Buffer[0] = value;
  I2C_Write_USB_PD(DPM_PDO_NUMB, Buffer,1);
}

void STUSB4500::setExternalPower(uint8_t value)
{
  if(value != 0) value = 1;

  //load SNK_UNCONS_POWER (sector 3, byte 2, bit 3)
  sector[3][2] &= 0xF7; //clear bit 3
  sector[3][2] |= (value)<<3;
}

void STUSB4500::setUsbCommCapable(uint8_t value)
{
  if(value != 0) value = 1;

  //load USB_COMM_CAPABLE (sector 3, byte 2, bit 0)
  sector[3][2] &= 0xFE; //clear bit 0
  sector[3][2] |= (value);
}

void STUSB4500::setConfigOkGpio(uint8_t value)
{
  if(value < 2) value = 0;
  else if(value > 3) value = 3;

  //load POWER_OK_CFG (sector 4, byte 4, bits 5:6)
  sector[4][4] &= 0x9F; //clear bit 3
  sector[4][4] |= value<<5;
}

void STUSB4500::setGpioCtrl(uint8_t value)
{
  if(value > 3) value = 3;

  //load GPIO_CFG (sector 1, byte 0, bits 4:5)
  sector[1][0] &= 0xCF; //clear bits 4:5
  sector[1][0] |= value<<4;
}

void STUSB4500::setPowerAbove5vOnly(uint8_t value)
{
  if(value != 0) value = 1;

  //load POWER_ONLY_ABOVE_5V (sector 4, byte 6, bit 3)
  sector[4][6] &= 0xF7; //clear bit 3
  sector[4][6] |= (value<<3); //set bit 3
}

void STUSB4500::setReqSrcCurrent(uint8_t value)
{
  if(value != 0) value = 1;

  //load REQ_SRC_CURRENT (sector 4, byte 6, bit 4)
  sector[4][6] &= 0xEF; //clear bit 4
  sector[4][6] |= (value<<4); //set bit 4
}

void STUSB4500::softReset( void )
{
  uint8_t Buffer[1];

  //Soft Reset
  Buffer[0] = 0x0D; //SOFT_RESET
  I2C_Write_USB_PD(TX_HEADER_LOW, Buffer,1);

  Buffer[0] = 0x26; //SEND_COMMAND
  I2C_Write_USB_PD(PD_COMMAND_CTRL, Buffer,1);
}

uint32_t STUSB4500::readPDO(uint8_t pdo_numb)
{
  uint32_t pdoData=0;
  uint8_t Buffer[4];

  //PDO1:0x85, PDO2:0x89, PDO3:0x8D
  I2C_Read_USB_PD(0x85 + ((pdo_numb-1)*4), Buffer, 4);

  //Combine the 4 buffer bytes into one 32-bit integer
  for(uint8_t i=0; i<4; i++)
  {
    uint32_t tempData = Buffer[i];
    tempData = (tempData<<(i*8));
    pdoData += tempData;
  }

  return pdoData;
}

void STUSB4500::writePDO(uint8_t pdo_numb, uint32_t pdoData)
{
  uint8_t Buffer[4];

  Buffer[0] = (pdoData)    & 0xFF;
  Buffer[1] = (pdoData>>8) & 0xFF;
  Buffer[2] = (pdoData>>16)& 0xFF;
  Buffer[3] = (pdoData>>24)& 0xFF;

  I2C_Write_USB_PD(0x85 + ((pdo_numb-1)*4), Buffer, 4);
}

uint8_t STUSB4500::CUST_EnterWriteMode(unsigned char ErasedSector)
{
  uint8_t Buffer[1];


  Buffer[0]=FTP_CUST_PASSWORD;   /* Set Password*/
  if ( I2C_Write_USB_PD(FTP_CUST_PASSWORD_REG,Buffer,1) != 0 )return -1;

  Buffer[0]= 0 ;   /* this register must be NULL for Partial Erase feature */
  if ( I2C_Write_USB_PD(RW_BUFFER,Buffer,1) != 0 )return -1;

  {
    //NVM Power-up Sequence
    //After STUSB start-up sequence, the NVM is powered off.

    Buffer[0]= 0;  /* NVM internal controller reset */
    if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1)  != 0 ) return -1;

    Buffer[0]= FTP_CUST_PWR | FTP_CUST_RST_N; /* Set PWR and RST_N bits */
    if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1) != 0 ) return -1;
  }


  Buffer[0]=((ErasedSector << 3) & FTP_CUST_SER) | ( WRITE_SER & FTP_CUST_OPCODE) ;  /* Load 0xF1 to erase all sectors of FTP and Write SER Opcode */
  if ( I2C_Write_USB_PD(FTP_CTRL_1,Buffer,1) != 0 )return -1; /* Set Write SER Opcode */

  Buffer[0]=FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ ;
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1)  != 0 )return -1; /* Load Write SER Opcode */

  do
  {
      delay(500);
      if ( I2C_Read_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1; /* Wait for execution */
  }
  while(Buffer[0] & FTP_CUST_REQ);

  Buffer[0]=  SOFT_PROG_SECTOR & FTP_CUST_OPCODE ;
  if ( I2C_Write_USB_PD(FTP_CTRL_1,Buffer,1) != 0 )return -1;  /* Set Soft Prog Opcode */

  Buffer[0]=FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ ;
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1)  != 0 )return -1; /* Load Soft Prog Opcode */

  do
  {
    if ( I2C_Read_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1; /* Wait for execution */
  }
  while(Buffer[0] & FTP_CUST_REQ);

  Buffer[0]= ERASE_SECTOR & FTP_CUST_OPCODE ;
  if ( I2C_Write_USB_PD(FTP_CTRL_1,Buffer,1) != 0 )return -1; /* Set Erase Sectors Opcode */

  Buffer[0]=FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ ;
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1)  != 0 )return -1; /* Load Erase Sectors Opcode */

  do
  {
    if ( I2C_Read_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1; /* Wait for execution */
  }
  while(Buffer[0] & FTP_CUST_REQ);

  return 0;
}

uint8_t STUSB4500::CUST_ExitTestMode(void)
{
  uint8_t Buffer[2];

  Buffer[0]= FTP_CUST_RST_N;
  Buffer[1]= 0x00;  /* clear registers */
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1;

  Buffer[0]= 0x00;
  if ( I2C_Write_USB_PD(FTP_CUST_PASSWORD_REG,Buffer,1) != 0 )return -1;  /* Clear Password */

  return 0 ;
}

uint8_t STUSB4500::CUST_WriteSector(char SectorNum, unsigned char *SectorData)
{
  uint8_t Buffer[1];

  //Write the 64-bit data to be written in the sector
  if ( I2C_Write_USB_PD(RW_BUFFER,SectorData,8) != 0 )return -1;

  Buffer[0]=FTP_CUST_PWR | FTP_CUST_RST_N; /*Set PWR and RST_N bits*/
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1;

  //NVM Program Load Register to write with the 64-bit data to be written in sector
  Buffer[0]= (WRITE_PL & FTP_CUST_OPCODE); /*Set Write to PL Opcode*/
  if ( I2C_Write_USB_PD(FTP_CTRL_1,Buffer,1) != 0 )return -1;

  Buffer[0]=FTP_CUST_PWR |FTP_CUST_RST_N | FTP_CUST_REQ;  /* Load Write to PL Sectors Opcode */
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1;

  do
  {
    if ( I2C_Read_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1; /* Wait for execution */
  }
  while(Buffer[0] & FTP_CUST_REQ) ; //FTP_CUST_REQ clear by NVM controller


  //NVM "Word Program" operation to write the Program Load Register in the sector to be written
  Buffer[0]= (PROG_SECTOR & FTP_CUST_OPCODE);
  if ( I2C_Write_USB_PD(FTP_CTRL_1,Buffer,1) != 0 )return -1;/*Set Prog Sectors Opcode*/

  Buffer[0]=(SectorNum & FTP_CUST_SECT) |FTP_CUST_PWR |FTP_CUST_RST_N | FTP_CUST_REQ;
  if ( I2C_Write_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1; /* Load Prog Sectors Opcode */

  do
  {
    if ( I2C_Read_USB_PD(FTP_CTRL_0,Buffer,1) != 0 )return -1; /* Wait for execution */
  }
  while(Buffer[0] & FTP_CUST_REQ); //FTP_CUST_REQ clear by NVM controller

  return 0;
}

uint8_t STUSB4500::I2C_Write_USB_PD(uint16_t Register ,uint8_t *DataW ,uint16_t Length)
{
  uint8_t error=0;
  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(Register);
  for(uint8_t i=0;i<Length;i++)
  {
    _i2cPort->write(*(DataW+i));
  }
  error = _i2cPort->endTransmission();
  delay(1);

  return error;
}

uint8_t STUSB4500::I2C_Read_USB_PD(uint16_t Register ,uint8_t *DataR ,uint16_t Length)
{
  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(Register);
  _i2cPort->endTransmission();
  _i2cPort->requestFrom(_deviceAddress,Length);
  uint8_t tempData[Length];
  for(uint16_t i=0;i<Length;i++)
  {
    tempData[i] = _i2cPort->read();
  }
  memcpy(DataR,tempData,Length);

  return 0;
}

bool STUSB4500::wireRead(uint16_t addr, uint8_t *buff, uint16_t size) {
    I2C_Read_USB_PD(addr, buff, size);
    return true;
}

bool STUSB4500::wireWrite(uint16_t addr, uint8_t *buff, uint16_t size) {
    I2C_Write_USB_PD(addr, buff, size);
    return true;
}


// CableStatus STUSB4500::cableStatus(void) const
CableStatus STUSB4500::cableStatus(void) {
    uint8_t portStatus;
    uint8_t typeCStatus;

    if (!wireRead(REG_PORT_STATUS, &portStatus, 1U)) {
        return CableStatus::NONE;
    }

    if (VALUE_ATTACHED == (portStatus & STUSBMASK_ATTACHED_STATUS)) {
        if (!wireRead(REG_TYPE_C_STATUS, &typeCStatus, 1U)) {
            return CableStatus::NONE;
        }

        if (0U == (typeCStatus & MASK_REVERSE)) {
            return CableStatus::CC1Connected;
        } else {
            return CableStatus::CC2Connected;
        }
    } else {
        return CableStatus::NotConnected;
    }
}

void parseOtherPDO(uint32_t pdo) {
    USB_PD_SRC_PDOTypeDef srcPdo = {};
    srcPdo.d32 = pdo;

    int i;
    int PDO_V, PDO_V_Min, PDO_V_Max;
    int PDO_I;
    float PDO_P;
    float MAX_POWER = 0;

    // Log.printf("\r\n---- Usb_Port #%i: Read PDO from SOURCE ---------\r\n", Usb_Port);
    // int num_pdo = PDO_FROM_SRC_Num[Usb_Port];
    // for (i = 0; i < PDO_FROM_SRC_Num[Usb_Port]; i++) // Check that PDO is not a APDO
    // {
    //     if ((PDO_FROM_SRC[Usb_Port][i].d32 & 0xC0000000) != 0x00000000)
    //         num_pdo--;
    // }
    // PDO_FROM_SRC_Num_Sel[Usb_Port] = num_pdo;

    // Log.printf("%i Objects => %i fixed PDO:\r\n", PDO_FROM_SRC_Num[Usb_Port], PDO_FROM_SRC_Num_Sel[Usb_Port]);
    // for (i = 0; i < PDO_FROM_SRC_Num[Usb_Port]; i++) {
        switch (srcPdo.fix.FixedSupply) {
        case 0: /* fixed supply */
        {
            PDO_V = (srcPdo.fix.Voltage) * 50;
            PDO_I = (srcPdo.fix.Max_Operating_Current) * 10;
            PDO_P = PDO_V * PDO_I;
            Log.printf(" - PDO%u (fixed)     = (%4.2fV, %4.2fA, = %4.1fW)\r\n", i + 1, (float)PDO_V / 1000.0, (float)PDO_I / 1000.0, PDO_P / 1000000.0);
            if (PDO_P >= MAX_POWER) {
                MAX_POWER = PDO_P / 1000000.0;
            }
        } break;
        case 2: /* Variable Supply */
        {
            PDO_V_Max = (srcPdo.var.Max_Voltage) * 50;
            PDO_V_Min = (srcPdo.var.Min_Voltage) * 50;
            PDO_I = (srcPdo.var.Operating_Current) * 10;
            Log.printf(" - PDO%u (variable)  = (%4.2fV, %4.2fV, = %4.2fA) not selectable\r\n", i + 1, (float)PDO_V_Min / 1000.0, (float)PDO_V_Max / 1000.0, (float)PDO_I / 1000.0);
            (void)(PDO_V_Min);
            PDO_P = PDO_V_Max * PDO_I / 10000000;
            if (PDO_P >= MAX_POWER) {
                MAX_POWER = PDO_P;
            }

        } break;
        case 1: /* Battery Supply */
        {
            PDO_V_Max = (srcPdo.bat.Max_Voltage) * 50;
            PDO_V_Min = (srcPdo.bat.Min_Voltage) * 50;
            PDO_P = (srcPdo.bat.Operating_Power) * 250;
            Log.printf(" - PDO%u (battery)   = (%4.2fV, %4.2fV, = %4.1fW) not selectable\r\n", i + 1, (float)PDO_V_Min / 1000.0, (float)PDO_V_Max / 1000.0, (float)PDO_P / 1000);
            if ((float)PDO_P / 1000 >= MAX_POWER) {
                MAX_POWER = PDO_P / 1000;
            }
        } break;
        case 3: /* Augmented Supply */
        {
            PDO_V_Max = (srcPdo.apdo.Max_Voltage) * 100;
            PDO_V_Min = (uint8_t)(srcPdo.apdo.Min_Voltage) * 100;
            PDO_I = (srcPdo.apdo.Max_Current) * 50;
            PDO_P = (float)(PDO_V_Max * PDO_I) / 1000000;
            Log.printf(" - PDO (augmented) = (%4.2fV, %4.2fV, %4.2fA = %4.1fW) => APDO not selectable \r\n", i + 1, (float)PDO_V_Min / 1000.0, (float)PDO_V_Max / 1000.0,
                   (float)PDO_I / 1000.0, PDO_P);
            if (PDO_P >= MAX_POWER) {
                MAX_POWER = PDO_P;
            }
        } break;
        }
    // }
    Log.printf("P(max)=%4.1fW\r\n", MAX_POWER);
}

// bool printRDO();
void STUSB4500::printRDO() {
    // RX_HEADER
    USBPDMessageHeader rxHeader = {};
    uint32_t rdo[7] = {};
    wireRead(RX_HEADER, (uint8_t *)&rxHeader, 2);
    wireRead(RX_DATA_OBJ, (uint8_t *)rdo, sizeof(rdo));

    Log.info("messageType: %d", rxHeader.b.messageType);
    Log.info("portDataRole: %d", rxHeader.b.portDataRole);
    Log.info("specRevision: %d", rxHeader.b.specRevision);
    Log.info("messageId: %d", rxHeader.b.messageID);
    Log.info("numberOfDataObjects: %d", rxHeader.b.dataObjectCount);
    Log.info("extended: %d", rxHeader.b.extended);

    for (int i = 0; i < 7; i++) {
        parseOtherPDO(rdo[i]);
    }

    Log.info("");
    Log.info("");
}