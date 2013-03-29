/**
  ******************************************************************************
  * @file    sst25vf_spi.c
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    29-March-2013
  * @brief   This file contains all the functions prototypes for the 
  *          sst25vf_spi Flash firmware driver.
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "sst25vf_spi.h"

static uint32_t Device_ID = 0;

/**
  * @brief  Sends a byte through the SPI interface and return the byte received
  *         from the SPI bus.
  * @param  data: byte to send.
  * @retval The value of the received byte.
  */
static uint8_t sFLASH_SendByte(uint8_t data)
{
    //Wait until the transmit buffer is empty
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    // Send the byte
    SPI_I2S_SendData(SPI1, data);

    //Wait until a data is received
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    // Get the received data
    data = SPI_I2S_ReceiveData(SPI1);

    // Return the shifted data
    return data;
}

static uint8_t sFLASH_ReadStatus(void)
{
    uint8_t tmp;

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_RDSR );
    tmp=sFLASH_SendByte(0XFF);
    sFLASH_CS_HIGH();
    return tmp;
}

static void sFLASH_WaitBusy(void)
{
    while( sFLASH_ReadStatus() & (0x01));
}

/**
 * @brief read [size] byte from [offset] to [buffer]
 * @param offset uint32_t unit : byte
 * @param buffer uint8_t*
 * @param size uint32_t   unit : byte
 * @return uint32_t byte for read
 *
 */
uint32_t sFLASH_Read(uint32_t offset,uint8_t * buffer,uint32_t size)
{
    uint32_t index;

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_WRDI );
    sFLASH_CS_HIGH();

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_READ);
    sFLASH_SendByte(  offset>>16 );
    sFLASH_SendByte(  offset>>8 );
    sFLASH_SendByte(  offset );
    for(index=0; index<size; index++)
    {
        *buffer++ = sFLASH_SendByte(0xFF);
    }
    sFLASH_CS_HIGH();

    return size;
}

/**
 * @brief write N page on [page]
 * @param page uint32_t unit : byte (4096 * N,1 page = 4096byte)
 * @param buffer const uint8_t*
 * @param size uint32_t unit : byte ( 4096*N )
 * @return uint32_t
 *
 */
uint32_t sFLASH_WritePage(uint32_t page,const uint8_t * buffer,uint32_t size)
{
    uint32_t index;

    page &= ~0xFFF; // page size = 4096byte

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_WREN );//write en
    sFLASH_CS_HIGH();

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_ERASE_4K );
    sFLASH_SendByte( page >> 16 );
    sFLASH_SendByte( page >> 8 );
    sFLASH_SendByte( page  );
    sFLASH_CS_HIGH();

    sFLASH_WaitBusy(); // wait erase done.

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_WREN );//write en
    sFLASH_CS_HIGH();

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_AAIP );
    sFLASH_SendByte(  page>>16 );
    sFLASH_SendByte(  page>>8 );
    sFLASH_SendByte(  page );

    sFLASH_SendByte( *buffer++ );
    sFLASH_SendByte( *buffer++ );
    size -= 2;
    sFLASH_CS_HIGH();

    sFLASH_WaitBusy();

    for(index=0; index < size/2; index++)
    {
        sFLASH_CS_LOW();
        sFLASH_SendByte( sFLASH_CMD_AAIP );
        sFLASH_SendByte( *buffer++ );
        sFLASH_SendByte( *buffer++ );
        sFLASH_CS_HIGH();
        sFLASH_WaitBusy();
    }
    sFLASH_CS_HIGH();

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_WRDI );
    sFLASH_CS_HIGH();

    return size;
}

/**
 * @brief SPI flash init
 * @param void
 * @return void
 *
 */
void sFLASH_Init(void)
{
	sFLASH_SPI_Init();

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_WRDI );
    sFLASH_CS_HIGH();

    sFLASH_CS_LOW();
    sFLASH_SendByte( sFLASH_CMD_JEDEC_ID );
    Device_ID  = sFLASH_SendByte(0xFF);
    Device_ID |= sFLASH_SendByte(0xFF)<<8;
    Device_ID |= sFLASH_SendByte(0xFF)<<16;
    sFLASH_CS_HIGH();

    if(Device_ID == sFLASH_SST25VF040_ID)
    {
        sFLASH_CS_LOW();
        sFLASH_SendByte( sFLASH_CMD_DBSY );
        sFLASH_CS_HIGH();

        sFLASH_CS_LOW();
        sFLASH_SendByte( sFLASH_CMD_EWSR );
        sFLASH_CS_HIGH();

        sFLASH_CS_LOW();
        sFLASH_SendByte( sFLASH_CMD_WRSR );
        sFLASH_SendByte( 0 );
        sFLASH_CS_HIGH();
    }
}

