/**
 ******************************************************************************
 * @file    Ymodem.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    28-July-2014
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIB_YMODEM_H
#define __LIB_YMODEM_H

#include "system_ymodem.h"
#include "system_update.h"

class Ymodem
{
public:
    Ymodem();
};

Ymodem::Ymodem()
{
    set_ymodem_serial_flash_update_handler(Ymodem_Serial_Flash_Update);
}

Ymodem ymodem;

#endif  /* __LIB_YMODEM_H */
