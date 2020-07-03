/**
 ******************************************************************************
 * @file    i2c_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "system_error.h"

int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config)
{
  return SYSTEM_ERROR_NONE;
}

void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved)
{
}

void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved)
{
}

void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved)
{
}

void hal_i2c_end(hal_i2c_interface_t i2c,void* reserved)
{
}

uint32_t hal_i2c_request(hal_i2c_interface_t i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved)
{
  return SYSTEM_ERROR_NONE;
}

int32_t hal_i2c_request_ex(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config, void* reserved)
{
    return 0;
}

void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config)
{
}

uint8_t hal_i2c_end_transmission(hal_i2c_interface_t i2c, uint8_t stop,void* reserved)
{
  return SYSTEM_ERROR_NONE;
}

uint32_t hal_i2c_write(hal_i2c_interface_t i2c, uint8_t data,void* reserved)
{
  return SYSTEM_ERROR_NONE;
}

int32_t hal_i2c_available(hal_i2c_interface_t i2c,void* reserved)
{
  return SYSTEM_ERROR_NONE;
}

int32_t hal_i2c_read(hal_i2c_interface_t i2c,void* reserved)
{
  return SYSTEM_ERROR_NONE;
}

int32_t hal_i2c_peek(hal_i2c_interface_t i2c,void* reserved)
{
    return SYSTEM_ERROR_NONE;
}

void hal_i2c_flush(hal_i2c_interface_t i2c,void* reserved)
{
  // XXX: to be implemented.
}

bool hal_i2c_is_enabled(hal_i2c_interface_t i2c,void* reserved)
{
    return false;
}

void hal_i2c_set_callback_on_received(hal_i2c_interface_t i2c, void (*function)(int),void* reserved)
{

}

void hal_i2c_set_callback_on_requested(hal_i2c_interface_t i2c, void (*function)(void),void* reserved)
{

}

/*******************************************************************************
 * Function Name  : HAL_I2C1_EV_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Event interrupt request(Only for Slave mode).
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_EV_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : HAL_I2C1_ER_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Error interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_ER_Handler(void)
{
}

void hal_i2c_enable_dma_mode(hal_i2c_interface_t i2c, bool enable,void* reserved)
{
}

uint8_t hal_i2c_reset(hal_i2c_interface_t i2c, uint32_t reserved, void* reserved1)
{
  return SYSTEM_ERROR_NONE;
}

int HAL_I2C_Sleep(hal_i2c_interface_t i2c, bool sleep, void* reserved) {
    return 0;
}

int32_t hal_i2c_lock(hal_i2c_interface_t i2c, void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int32_t hal_i2c_unlock(hal_i2c_interface_t i2c, void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
