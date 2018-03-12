#include "wiced_resource.h"
#include "platform_resource.h"
#include <stddef.h>
#include "debug.h"

#include "spi_flash.h"
#include "flash_mal.h"

__attribute__((weak)) resource_result_t platform_read_external_resource( const resource_hnd_t* resource, uint32_t offset, uint32_t maxsize, uint32_t* size, void* buffer ) {

	uint32_t address = EXTERNAL_FLASH_WIFI_FIRMWARE_ADDRESS + offset;

	sFLASH_ReadBuffer(buffer, address, maxsize);

	*size = maxsize;

    return RESOURCE_SUCCESS;
}
