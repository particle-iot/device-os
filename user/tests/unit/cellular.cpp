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

#include "cellular_internal.h"

#undef WARN
#undef INFO
#include "catch.hpp"

using namespace detail;

SCENARIO("IMSI range should default to Telefonica as Network Provider", "[cellular]") {
    REQUIRE(_cellular_imsi_to_network_provider(NULL) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("") == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("123456789012345") == CELLULAR_NETPROV_TELEFONICA);
}

SCENARIO("IMSI range should set Twilio as Network Provider", "[cellular]") {
    REQUIRE(_cellular_imsi_to_network_provider("310260859000000") == CELLULAR_NETPROV_TWILIO);
    REQUIRE(_cellular_imsi_to_network_provider("310260859500000") == CELLULAR_NETPROV_TWILIO);
    REQUIRE(_cellular_imsi_to_network_provider("310260859999999") == CELLULAR_NETPROV_TWILIO);
}
