/**
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

#include "catch.hpp"

#include "protocol.h"

using namespace particle::protocol;

class AbstractProtocol : public Protocol
{
public:
	AbstractProtocol(MessageChannel& channel) : Protocol(channel) {}

	virtual size_t build_hello(Message& message, bool was_ota_upgrade_successful)
	{
		return 0;
	}

	virtual void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor)
	{
	}

};

SCENARIO("default product co-ordinates are set")
{
	MessageChannel* channel = nullptr;
	AbstractProtocol p(*channel);	// channel is not used
	product_details_t details;
	details.size = sizeof(details);
	p.get_product_details(details);

	REQUIRE(details.product_id==PRODUCT_ID);
	REQUIRE(details.product_version==PRODUCT_FIRMWARE_VERSION);
}



