
#ifndef SYSTEM_UPDATE_H
#define	SYSTEM_UPDATE_H

#include "spark_wiring_stream.h"
#include <stdint.h>

typedef bool (*ymodem_serial_flash_update_handler)(Stream *serialObj, uint32_t sFlashAddress);
void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler);

bool system_serialSaveFile(Stream *serialObj, uint32_t sFlashAddress);

bool system_serialFirmwareUpdate(Stream *serialObj);

#endif	/* SYSTEM_UPDATE_H */

