/**
 ******************************************************************************
 Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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
#include "fakeit.hpp"
#include "coap_channel.h"
#include "forward_message_channel.h"

using namespace particle::protocol;
using namespace fakeit;

class ForwardCoAPChannel: public CoAPChannel<ForwardMessageChannel>
{
	using super = CoAPChannel<ForwardMessageChannel>;
public:

	ForwardCoAPChannel(MessageChannel& ch)
	{
		setForward(&ch);
	}
};

class ForwardCoAPReliability: public CoAPReliability<ForwardMessageChannel>
{
	using super = CoAPReliability<ForwardMessageChannel>;
public:

	ForwardCoAPReliability(MessageChannel& ch)
	{
		setForward(&ch);
	}

};

message_id_t decode_id(Message& msg)
{
	uint8_t* buf = msg.buf();
	return buf[2] << 8 | buf[3];
}

SCENARIO("message IDs monotinically increment from 1")
{
	GIVEN("a CoAP message channel and a mock message")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPChannel channel(mock.get());

		uint8_t buf[10];
		When(Method(mock,create)).AlwaysDo([&buf](Message& msg, size_t len)
				{
					msg.set_buffer(buf, 10); return NO_ERROR;
				});
		When(Method(mock,send)).AlwaysReturn(NO_ERROR);

		WHEN("a message is sent")
		{
			Message msg;
			CHECK(channel.create(msg, 5)==NO_ERROR);
			REQUIRE(channel.send(msg)==NO_ERROR);
			THEN("the encoded message ID is 1")
			{
				REQUIRE(decode_id(msg)==1);
			}
			AND_WHEN("a second message is sent")
			{
				Message msg;
				CHECK(channel.create(msg, 5)==NO_ERROR);
				REQUIRE(channel.send(msg)==NO_ERROR);
				THEN("the encoded message ID is 2")
				{
					REQUIRE(decode_id(msg)==2);
				}

				AND_WHEN("300 more messages are sent")
				{
					Message msg;
					for (int i=0; i<300; i++)
					{
						channel.create(msg, 5);
						channel.send(msg);
					}
					THEN("the encoded message ID is 302")
					{
						REQUIRE(decode_id(msg)==302);
					}
				}
			}
		}
	}
}


SCENARIO("there are no registered messages initially")
{
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPReliability store(mock.get());

		THEN("no messages are present")
		{
			REQUIRE(store.from_id(0)==nullptr);
			REQUIRE(store.from_id(1)==nullptr);
			REQUIRE(store.from_id(2)==nullptr);
		}
	}
}

SCENARIO("a message can be added and removed by id", "[reliability]")
{
	const message_id_t id = 456;
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPReliability store(mock.get());
		REQUIRE(store.from_id(id)==nullptr);

		WHEN("a message is added with id 456")
		{
			CoAPMessage message(id);
			REQUIRE(store.add(message)==NO_ERROR);
			THEN("the message can be retrieved by id 456")
			{
				CoAPMessage* retrieve = store.from_id(id);
				REQUIRE(retrieve==&message);

				AND_WHEN("the message is removed")
				{
					CoAPMessage* removed = store.remove(id);
					THEN("the removed message is the one added")
					{
						REQUIRE(removed==&message);
						REQUIRE(message.get_next()==nullptr);
						AND_WHEN("the same id is removed again") {
							removed = store.remove(id);
							THEN("no message is retrieved") {
								REQUIRE(store.remove(id)==nullptr);
							}
						}
					}
				}
			}
		}
	}
}

SCENARIO("a message can be added with the same id more than once", "[reliability]")
{
	const message_id_t id = 456;
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPReliability store(mock.get());
		REQUIRE(store.from_id(id)==nullptr);

		WHEN("a message is added twice")
		{
			CoAPMessage m1(id);
			CoAPMessage m2(id);

			CHECK(store.add(m1)==NO_ERROR);
			REQUIRE(store.add(m2)==NO_ERROR);
			THEN("retrieving a the message by id returns the last added message")
			{
				REQUIRE(store.from_id(id)==&m2);
				AND_WHEN("the message is removed")
				{
					CHECK(store.remove(id)==&m2);
					THEN("no message is retrieved")
					{
						REQUIRE(store.from_id(id)==nullptr);
					}
				}
			}
		}
	}
}

SCENARIO("multiple messages are stored in a list in the order they are added, most recent first")
{
	const message_id_t id1 = 456;
	const message_id_t id2 = 345;
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPReliability store(mock.get());
		REQUIRE(store.from_id(id1)==nullptr);
		REQUIRE(store.from_id(id2)==nullptr);

		WHEN("adding two messages with distinct IDs")
		{
			CoAPMessage m1(id1), m2(id2);
			REQUIRE(m1.get_id()==id1);
			REQUIRE(m2.get_id()==id2);
			REQUIRE(store.add(m1)==NO_ERROR);
			REQUIRE(store.add(m2)==NO_ERROR);

			THEN("the second message points to the first")
			{
				REQUIRE(m2.get_next()==&m1);
				AND_THEN("both messages can be retrieved")
				{
					REQUIRE(store.from_id(id1)==&m1);
					REQUIRE(store.from_id(id2)==&m2);
				}
			}
			AND_WHEN("removing the most recently added message")
			{
				REQUIRE(store.remove(id2)==&m2);
				THEN("only that message is removed")
				{
					CHECK(m2.get_next()==nullptr);
					CHECK(store.from_id(id2)==nullptr);
					CHECK(store.from_id(id1)==&m1);
				}
			}
		}
	}
}

SCENARIO("adding the same message twice")
{
	const message_id_t id = 456;
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPReliability store(mock.get());
		REQUIRE(store.from_id(id)==nullptr);
		CoAPMessage m(id);
		WHEN("the message is added twice")
		{
			CHECK(store.add(m)==NO_ERROR);
			THEN("the second add succeeds")
			{
				REQUIRE(store.add(m)==NO_ERROR);
			}
		}
	}
}

SCENARIO("an unacknowledged message is resent up to 4 times after which it is removed")
{
	FAIL("todo");
}

SCENARIO("a Confirmable message is pending until acknowledged or timeout")
{
	GIVEN("a reliable channel and a confirmable message")
	{
		Mock<MessageChannel> mock;
		ForwardCoAPReliability store(mock.get());
		uint8_t buf[10];
		When(Method(mock,create)).AlwaysDo([&buf](Message& msg, size_t len)
			{
				msg.set_buffer(buf, 10); return NO_ERROR;
			});
		When(Method(mock,send)).AlwaysReturn(NO_ERROR);

		Message m;
		store.create(m, 5);
		uint8_t data[] = { 0x45, 0, 0, 0 };
		memcpy(m.buf(), data, 4);

		WHEN("the message is sent")
		{
			store.send(m);
			THEN("the message is pending")
			{
				message_id_t id = decode_id(m);
				REQUIRE(id!=0);
				REQUIRE(store.from_id(id)!=nullptr);
			}
			AND_WHEN("the message is acknowledged")
			{
				THEN("the message is no longer pending")
				{

				}
			}
		}
	}
}
