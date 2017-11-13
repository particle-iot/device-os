/**
 ******************************************************************************
 * @file    spark_wiring_network.h
 * @author  Satish Nair, Timothy Brown
 * @version V1.0.0
 * @date    18-Mar-2014
 * @brief   Header for spark_wiring_network.cpp module
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

#ifndef __SPARK_WIRING_NETWORK_H
#define __SPARK_WIRING_NETWORK_H

#include "spark_wiring_ipaddress.h"
#include "system_network.h"

namespace spark {

class NetworkClass;

// Defined as the primary network
extern NetworkClass& Network;


//Retained for compatibility and to flag compiler warnings as build errors
class NetworkClass
{
public:
    bool ready(void) {
        return network_ready(*this, 0, nullptr);
    }

    void on(void) {
        network_on(*this, 0, 0, nullptr);
    }

    void off(void) {
        network_off(*this, 0, 0, nullptr);
    }

    virtual operator network_interface_t()=0;


    static NetworkClass& from(network_interface_t nif);
};


}

#endif
