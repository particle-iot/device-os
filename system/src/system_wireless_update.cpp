
#if (PLATFORM_ID==88)

#include "system_update.h"
#include "system_wireless_update.h"
#include "file_transfer.h"

FileTransfer::Descriptor fd;

static uint32_t recieved_size = 0;
static uint32_t addr_offset = 0;
static bool ota_init = false;

void Wireless_Update_Begin(uint32_t file_length, uint16_t chunk_size, uint32_t chunk_address, uint8_t file_store)
{
	recieved_size = 0;
	addr_offset = 0;

    if(file_store == 1)
        fd.store = FileTransfer::Store::SYSTEM; // FAC firmware and WiFi firmware are stored in external flash
    else
        fd.store = FileTransfer::Store::FIRMWARE;
	
	fd.chunk_address = chunk_address;
	fd.chunk_size = chunk_size;
	fd.file_length = file_length;// Judge the length if valid or not

	Spark_Prepare_For_Firmware_Update(fd, 0, NULL);

	ota_init = true;
}

uint8_t Wireless_Update_Save_Chunk(uint8_t *data, uint16_t length)
{
    if(!ota_init)
    	return 2;

    fd.chunk_address = fd.file_address + addr_offset;
    fd.chunk_size = length;
    Spark_Save_Firmware_Chunk(fd, data, NULL);

    addr_offset += length;
    recieved_size += length;

    if(recieved_size >= fd.file_length)
    {
        ota_init = false;
        Spark_Finish_Firmware_Update(fd, recieved_size>0 ? 1 : 0, NULL);
        return 1;
    }
	
    return 0;
}

void Wireless_Update_Finish(void)
{
    HAL_WLAN_notify_OTA_update_completed();
}

#endif  /* PLATFORM_ID==88 */
