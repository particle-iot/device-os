/*
 ******************************************************************************
  Copyright (c) 2014-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SPARK_WIRING_CELLULAR_H
#define	SPARK_WIRING_CELLULAR_H

#include "spark_wiring_platform.h"

#if Wiring_Cellular

class CellularClass : public NetworkClass
{
    CellularDevice device;

public:

    void on() override { cellular_on(NULL); }
    void off() override { cellular_off(NULL); }
    void connect() override { cellular_connect(CellularConnect* connect, NULL); }
    void disconnect() override { cellular_disconnect(NULL); }
};


extern CellularClass Cellular;


#endif
#endif	/* SPARK_WIRING_CELLULAR_H */

