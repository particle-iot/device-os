/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include "wiringSerial.h"

// The Raspberry pi 3 has changed things a bit and you might need to add the option enable_uart=1 at the end of /boot/config.txt 
// From http://elinux.org/RPi_Serial_Connection

int serialFd;
bool peeking;
int peekedChar;

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
    serialFd = -1;
    peeking = false;
}

bool isSerialOpen() {
    return serialFd >= 0;
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
    serialFd = serialOpen("/dev/serial0", baud);
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    if (!isSerialOpen()) {
        return;
    }
    serialClose(serialFd);
    serialFd = -1;
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
    return 0;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    if (!isSerialOpen()) {
        return 0;
    }
    serialPutchar(serialFd, data);
    return 1;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    if (!isSerialOpen()) {
        return 0;
    }
    return serialDataAvail(serialFd) + (peeking ? 1 : 0);
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    if (HAL_USART_Available_Data(serial) <= 0) {
        return -1;
    }
    if (peeking) {
        peeking = false;
        return peekedChar;
    } else {
        return serialGetchar(serialFd);
    }
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
    if (!peeking) {
        peekedChar = HAL_USART_Read_Data(serial);
        if (peekedChar >= 0) {
            peeking = true;
        }
    }
    return peekedChar;
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
    if (!isSerialOpen()) {
        return;
    }
    serialFlush(serialFd);
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
    HAL_USART_Begin(serial, baud);
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data)
{
    return 0;
}
