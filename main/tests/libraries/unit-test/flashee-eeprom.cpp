/**
 * Copyright 2014  Matthew McGowan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "flashee-eeprom.h"

#if FLASHEE_FATFS_SUPPORT
#include "diskio.h"
#include "FlashIO.h"
#include "ff.h"

#ifndef FLASHEE_DEBUG_DISKIO
#define FLASHEE_DEBUG_DISKIO 0
#endif

#if FLASHEE_DEBUG_DISKIO
#include <stdio.h>
#define DEBUG_DISKIO(fmt, ...) printf(fmt"\r\n", __VA_ARGS__);
#else
#define DEBUG_DISKIO(...)
#endif

#endif


namespace Flashee {    
    
FlashDevice::~FlashDevice() { }

#ifdef SPARK
    static SparkExternalFlashDevice directFlash;
#else
    static FakeFlashDevice directFlash(512, 4096);
#endif

FlashDeviceRegion Devices::userRegion(directFlash, 0x80000, 0x200000);

}

/**
 * Compares data in the buffer with the data in flash.
 * @param data
 * @param address
 * @param length
 * @return
 */
#if 0
bool FlashDevice::comparePage(const void* data, flash_addr_t address, page_size_t length) {
    uint8_t buf[STACK_BUFFER_SIZE];
    page_size_t offset = 0;
    while (offset<length) {
        page_size_t toRead = min(sizeof(buf), length-offset);
        if (!readPage(buf, address+offset, toRead))
            break;
        if (!memcmp(buf, as_bytes(data)+offset, toRead))
            break;
        offset += toRead;
    }
    return offset==length;
}
#endif

#if FLASHEE_FATFS_SUPPORT

FRESULT Flashee::Devices::createFATRegion(flash_addr_t startAddress, flash_addr_t endAddress, 
    FATFS* pfs, FormatCmd formatCmd) {
    FlashDevice* device = createMultiPageEraseImpl(startAddress, endAddress, 2);
    if (device==NULL)
        return FR_INVALID_PARAMETER;
    device = new PageSpanFlashDevice(*device);
    return f_setFlashDevice(device, pfs, formatCmd);
}

static FlashDevice* fat_flash = NULL;


const page_size_t sector_size = 512;

bool is_formatted() {
    uint8_t sig[2];
    fat_flash->read(sig, 510, 2);
    return (sig[0]==0x55 && sig[1]==0xAA);
}

FRESULT low_level_format() {
    fat_flash->eraseAll();
    FRESULT result = f_mkfs("", 1, sector_size);
    if (result==FR_OK && !is_formatted())
        result = FR_DISK_ERR;        
    return result;
}

bool needs_low_level_format() {
    uint8_t sig[2];
    fat_flash->read(sig, 510, 2);
    return ((sig[0]!=0x55 && sig[1]!=0xAA) && (sig[0]&sig[1])!=0xFF);   
}

FRESULT f_setFlashDevice(FlashDevice* device, FATFS* pfs, FormatCmd cmd) {
    delete fat_flash;
    fat_flash = device;
    if (!fat_flash)
        return FR_OK;
    
    FRESULT result = f_mount(pfs, "", 0);
    if (result==FR_OK) {   
        bool formatRequired = cmd==Flashee::FORMAT_CMD_FORMAT || (cmd==Flashee::FORMAT_CMD_FORMAT_IF_NEEDED && !is_formatted());
        if (formatRequired) {
            result = low_level_format();
        }    
    }
    if (result==FR_OK) {
        FIL fil;
        result = f_open(&fil, "@@@@123~.tmp", FA_OPEN_EXISTING);        
        if (result==FR_NO_FILE)     // expected not to exist, if not formatted, will return FR_NO_FILESYSTEM
            result = FR_OK;
    }
    return result;
}

using namespace Flashee;

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
    DSTATUS status = STA_NOINIT;
    if (!pdrv) {            
        // determine if boot sector is present, if not, then erase area    
        if (needs_low_level_format()) {
            low_level_format();
        }
        status = 0;
    }
    DEBUG_DISKIO("disk_initialize(%d)->%d", pdrv, status);
    return status;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
    DSTATUS result = pdrv ? STA_NOINIT : 0;
    DEBUG_DISKIO("disk_status(%d)->%d", pdrv, result);
    return result;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
    DRESULT result = RES_PARERR;
	if (!pdrv) {
        result = fat_flash->read(buff, sector*sector_size, count*sector_size) ?
            RES_OK : RES_PARERR;
    }
    DEBUG_DISKIO("disk_read(%d, %x, %ul, %u)->%d", pdrv, buff, sector, count, result);
    return result;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	DRESULT result = RES_PARERR;
    if (!pdrv) {
        result = fat_flash->write(buff, sector*sector_size, count*sector_size) ?
            RES_OK : RES_PARERR;
    }        
    DEBUG_DISKIO("disk_write(%d, %x, %ul, %u)->%d", pdrv, buff, sector, count, result);
    return result;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    DWORD* dw = (DWORD*)buff;
    DRESULT result = RES_PARERR;
	switch (cmd) {
        case CTRL_SYNC: return RES_OK;
        case GET_SECTOR_COUNT:
            *dw = fat_flash->length()/sector_size;
            result = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            *dw = sector_size;
            result = RES_OK;
            break;
    }
	DEBUG_DISKIO("disk_ioctl(%d, %d, %d)->%d", pdrv, cmd, *dw, result);
    return result;
}
#endif

extern "C" {

DWORD get_fattime() {
#ifdef SPARK
    uint32_t now = Time.now();
    int year = Time.year(now);
    int month = Time.month(now);
    int day = Time.day(now);
    int hour = Time.hour(now);
    int min = Time.minute(now);
    int sec = Time.second(now);
    
    DWORD time = 
        ((year-1980)<<25) | (month<<21) | (day<<16) | (hour << 11) | (min << 5) | (sec/2);
    return time;
#else
    return 0;
#endif    
}

}

#endif
