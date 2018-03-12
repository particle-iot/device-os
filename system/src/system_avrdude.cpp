
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIB_AVRDUDE_H
#define __LIB_AVRDUDE_H

#if (PLATFORM_ID==88)

#include "spark_wiring.h"
#include "system_task.h"
#include "system_update.h"
#include "system_avrdude.h"
#include "ota_flash_hal.h"
#include "rgbled.h"
#include "file_transfer.h"
#include "spi_flash.h"

static Stream*  avrdudeSerial = NULL;
static uint32_t NAK_TIMEOUT  = 5000;

static uint8_t  rx_tx_buf[72];

static uint8_t  rw_flag;

static uint32_t block_size;
static uint32_t currentAddr;
static uint32_t page_addr;

static int32_t Avrdude_recieve_byte(uint8_t& c, uint32_t timeout)
{
    uint32_t start = HAL_Timer_Get_Milli_Seconds();
    while (HAL_Timer_Get_Milli_Seconds()-start <= timeout)
    {
        if (avrdudeSerial->available())
        {
            c = avrdudeSerial->read();
            return 0;
        }
    }
    return -1;
}

int32_t receive_firmware(FileTransfer::Descriptor& file)
{
    uint8_t c;
    uint8_t i;
    uint8_t buf[3];

    for(;;)
    {
        if(Avrdude_recieve_byte(c, NAK_TIMEOUT) != 0)
            return -1;

        switch(c)
        {
        case 'S' :
            rw_flag = 0;
            avrdudeSerial->print("RBL-DUO");
            break;
        case 'V' :
            avrdudeSerial->write((uint8_t)'1');
            avrdudeSerial->write((uint8_t)'0');
            break;
        case 'v' :
            avrdudeSerial->write((uint8_t)'1');
            avrdudeSerial->write((uint8_t)'0');
            break;
        case 'p' :
            avrdudeSerial->write((uint8_t)'S');
            break;
        case 'a' :
            avrdudeSerial->write((uint8_t)'Y');
            break;
        case 'b' : // Set block size. 0x0040 = 64bytes, it's limited by serial buffer size.
            avrdudeSerial->write((uint8_t)'Y');
            avrdudeSerial->write((uint8_t)0x00);
            avrdudeSerial->write((uint8_t)0x40);
            break;
        case 't' :
            avrdudeSerial->write((uint8_t)'D');
            avrdudeSerial->write((uint8_t)0x00);
            break;
        case 'T' :
            if(Avrdude_recieve_byte(c, NAK_TIMEOUT) !=0 )
                return -1;
            if(c == 'D')
                avrdudeSerial->write((uint8_t)0x0d);
            break;
        case 'P' :
            avrdudeSerial->write((uint8_t)0x0d);
            break;
        case 's' : // Platform ID
            avrdudeSerial->write((uint8_t)0x0B);
            avrdudeSerial->write((uint8_t)0x90);
            avrdudeSerial->write((uint8_t)0x1E);
            break;
        case 'L' :
            avrdudeSerial->write((uint8_t)0x0d);
            break;
        case 'E' :
            avrdudeSerial->write((uint8_t)0x0d);
            return 1;
            break;
        case 'A' :
            for (i=0; i<2; i++)
            {
                if (Avrdude_recieve_byte(buf[i], NAK_TIMEOUT) != 0)
                    return -1;
            }
            // Page address.
            page_addr  = (buf[0] << 9);
            page_addr |= (buf[1] << 1);
            if(page_addr == 0)
            {    // This is first block, initialize all variables.
                if(rw_flag == 0)
                {   // Fist it's the address of write.
                    // Second it's the address of read.
                    rw_flag = 1;

                    RGB.control(true);
                    RGB.color(RGB_COLOR_MAGENTA);
                    SPARK_FLASH_UPDATE = 1;
                }
                else if(rw_flag == 1)
                {
                    rw_flag = 0;
                }
                currentAddr = HAL_OTA_FlashAddress();
            }
            else
            {   // New page size.
                currentAddr = page_addr + HAL_OTA_FlashAddress();
            }

            if(rw_flag == 1)
            {   // The page of external flash is 4096.
                if((currentAddr & 0x00000FFF)==0)
                    HAL_FLASH_Begin(currentAddr, 4096, NULL);
            }
            TimingFlashUpdateTimeout = 0;

            avrdudeSerial->write((uint8_t)'\r');
            break;
        case 'g':
            // External flash readback.
            for (i=0; i<3; i++)
            {
                if (Avrdude_recieve_byte(buf[i], NAK_TIMEOUT) != 0)
                    return -1;
            }
            block_size = buf[0];
            block_size = (block_size<<8) + buf[1];

            if(buf[2] == 'F')
            {
                memset(rx_tx_buf, 0x00, block_size);

                LED_Toggle(LED_RGB);
                sFLASH_Init();
                sFLASH_ReadBuffer(rx_tx_buf, currentAddr, block_size);
                currentAddr += block_size;

                for(i=0; i<block_size; i++)
                    avrdudeSerial->write((uint8_t)rx_tx_buf[i]);
            }
            else if(buf[2] == 'E')
            {
                // Not support EEPROM.
                avrdudeSerial->write((uint8_t)'?');
            }
            break;
        case 'B' :
            for (i=0; i<3; i++)
            {
                if (Avrdude_recieve_byte(buf[i], NAK_TIMEOUT) != 0)
                    return -1;
            }
            // Block size.
            block_size = buf[0];
            block_size = (block_size<<8) + buf[1];

            if(buf[2] == 'F')
            {   // Write external flash
                uint32_t index;

                memset(rx_tx_buf, 0x00, block_size);
                // Get block.
                for (index=0; index<block_size; index++)
                {
                    if (Avrdude_recieve_byte(rx_tx_buf[index], NAK_TIMEOUT) != 0)
                        return -1;
                }

                // Write external flash.
                if(HAL_FLASH_Update(rx_tx_buf, currentAddr, block_size, NULL) != 0)
                    return -2;

                currentAddr += block_size;
                LED_Toggle(LED_RGB);
                avrdudeSerial->write((uint8_t)'\r');
            }
            else if(buf[2] == 'E')
            {
                avrdudeSerial->write((uint8_t)'?');
            }
            break;
        default:
            break;
        }
    }
}

bool Avrdude_Serial_Flash_Update(Stream *serialObj, FileTransfer::Descriptor& file, void* reserved)
{
    int32_t size;

    avrdudeSerial = serialObj;

    size = receive_firmware(file);
    if (size > 0)
    {
        avrdudeSerial->println("Downloaded file successfully!");
        avrdudeSerial->print("Size: ");
        avrdudeSerial->print(size);
        avrdudeSerial->println(" bytes");
        avrdudeSerial->flush();

        Spark_Finish_Firmware_Update(file, size>0 ? 1 : 0, NULL);
        return true;
    }
    else if (size == -1)
    {
        avrdudeSerial->println("Secieve timeout!");
    }

    else if (size == -2)
    {
        avrdudeSerial->println("Write flash fail!");
    }
    else if (size == -3)
    {
        avrdudeSerial->println("Firmware size fail!");
    }
    else
    {
        avrdudeSerial->println("Unknow fail!");
    }
    return false;
}

#endif  /* PLATFORM_ID==88 */

#endif  /* __LIB_AVRDUDE_H */
