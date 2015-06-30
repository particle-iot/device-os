/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "stm32f2xx.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

/* Extern variables ---------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/

void* dct_read_app_data(uint32_t offset);
int dct_write_app_data( const void* data, uint32_t offset, uint32_t size );

void* dct_read_app_data(uint32_t offset) {
    // return ((char*)wiced_dct_get_current_address(0))+offset;
    return 0;
}

int dct_write_app_data( const void* data, uint32_t offset, uint32_t size ) {
	// // first, let's try just writing the data
	// void* dct_start = dct_read_app_data(offset);
	// if (platform_write_flash_chunk((uint32_t)dct_start, data, size) != PLATFORM_SUCCESS)
	// 	return wiced_write_dct(DCT_section_offsets[0]+offset, data, size, 1, NULL);
	// else
	// 	return WICED_SUCCESS;
	return 0;
}
