/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include "wiringSerial.h"

// The Raspberry pi 3 has changed things a bit and you might need to add the option enable_uart=1 at the end of /boot/config.txt 
// From http://elinux.org/RPi_Serial_Connection

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
}

int serialFd = -1;

bool isSerialOpen() {
    return serialFd >= 0;
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
    serialFd = serialOpen("/dev/serial0", baud);
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    if (isSerialOpen()) {
        serialClose(serialFd);
        serialFd = -1;
    }
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
    return 0;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    if (isSerialOpen()) {
        serialPutchar(serialFd, data);
        return 1;
    }
    return 0;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    if (isSerialOpen()) {
        return serialDataAvail(serialFd);
    }
    return 0;
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    if (isSerialOpen()) {
        return serialGetchar(serialFd);
    }
    return 0;
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
    return 0;
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
    if (isSerialOpen()) {
        serialFlush(serialFd);
    }
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
    return isSerialOpen();
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void *ptr)
{
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data)
{
    return 0;
}
