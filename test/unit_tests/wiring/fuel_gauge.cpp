/**
 ******************************************************************************
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

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

#include "spark_wiring_fuel.h"

#undef WARN
#undef INFO
#include "catch2/catch.hpp"

using namespace particle::detail;

SCENARIO("Fuel Gauge VCELL_REGISTER conversion should return 1.25mV per bit", "[fuel_gauge]") {
    REQUIRE(_getVCell(0x00, 0x10) == (float)0.00125);
    REQUIRE(_getVCell(0x01, 0x00) == (float)(0.00125*16));
    REQUIRE(_getVCell(0x10, 0x00) == (float)(0.00125*256));
    REQUIRE(_getVCell(0xFF, 0xF0) == (float)(0.00125*4095));
}

SCENARIO("Fuel Gauge SoC conversion should return MSB.(LSB/256)%% by default", "[fuel_gauge]") {
    // MSB is the whole number
    // LSB is the decimal, resolution in units 1/256%
    // Same as 18-bit precision
    REQUIRE(_getSoC(0x01, 0x80) == (float)1.5);
    REQUIRE(_getSoC(0x10, 0x29) == (float)16.16015625);
    REQUIRE(_getSoC(0x64, 0x00) == (float)100.0);
    REQUIRE(_getSoC(0xFF, 0xFF) == (float)255.99609375);
}

SCENARIO("Fuel Gauge SoC conversion should return 18 bit values on demand", "[fuel_gauge]") {
    const byte soc_bits_precision = 18;
    REQUIRE(_getSoC(0x01, 0x80, soc_bits_precision) == (float)1.5);
    REQUIRE(_getSoC(0x10, 0x29, soc_bits_precision) == (float)16.16015625);
    REQUIRE(_getSoC(0x64, 0x00, soc_bits_precision) == (float)100.0);
    REQUIRE(_getSoC(0xFF, 0xFF, soc_bits_precision) == (float)255.99609375);
}

SCENARIO("Fuel Gauge SoC conversion should return 19 bit values on demand", "[fuel_gauge]") {
    const byte soc_bits_precision = 19;
    REQUIRE(_getSoC(0x01, 0x80, soc_bits_precision) == (float)0.75);
    REQUIRE(_getSoC(0x10, 0x29, soc_bits_precision) == (float)8.080078125);
    REQUIRE(_getSoC(0x64, 0x00, soc_bits_precision) == (float)50.0);
    REQUIRE(_getSoC(0xFF, 0xFF, soc_bits_precision) == (float)127.998046875);
}
