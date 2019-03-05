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

#include <climits>

#include "coap_channel.h"
#include "forward_message_channel.h"
#include "messages.h"

#include "catch.hpp"
#include "fakeit.hpp"

using namespace particle::protocol;
using namespace fakeit;

/**
 * A CoAPChannel that uses a forwarding message channel so we can redirect
 * where messages are delivered.
 */
class ForwardCoAPChannel: public CoAPChannel<ForwardMessageChannel>
{
	using super = CoAPChannel<ForwardMessageChannel>;
public:

	ForwardCoAPChannel(MessageChannel& ch)
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
	GIVEN("a CoAP message channel and a message with no assigned ID")
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
			msg.set_length(4);
			REQUIRE(channel.send(msg)==NO_ERROR);
			THEN("the encoded message ID is 1")
			{
				REQUIRE(decode_id(msg)==1);
				REQUIRE(msg.has_id());
				REQUIRE(msg.get_id()==1);
			}
			AND_WHEN("a second message is sent")
			{
				Message msg;
				CHECK(channel.create(msg, 5)==NO_ERROR);
				msg.set_length(4);
				REQUIRE(channel.send(msg)==NO_ERROR);
				THEN("the encoded message ID is 2")
				{
					REQUIRE(msg.has_id());
					REQUIRE(msg.get_id()==2);
					REQUIRE(decode_id(msg)==2);
				}

				AND_WHEN("300 more messages are sent")
				{
					for (int i=0; i<300; i++)
					{
						Message msg;
						channel.create(msg, 5);
						msg.set_length(4);
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
		CoAPMessageStore store;

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
		CoAPMessageStore store;
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
							CoAPMessage* removed2 = store.remove(id);
							THEN("no message is retrieved") {
								REQUIRE(removed2==nullptr);
								REQUIRE(store.remove(id)==nullptr);
							}
						}
					}
				}
			}
		}
	}
	REQUIRE(CoAPMessage::messages()==0);

}

SCENARIO("a message can be added to a message store with the same id more than once", "[reliability]")
{
	const message_id_t id = 456;
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		WHEN("a message is added twice")
		{
			CoAPMessageStore store;
			REQUIRE(store.from_id(id)==nullptr);

			CoAPMessage* m1 = new CoAPMessage(id);
			CoAPMessage* m2 = new CoAPMessage(id);

			CHECK(store.add(m1)==NO_ERROR);
			REQUIRE(store.add(m2)==NO_ERROR);
			THEN("retrieving a the message by id returns the last added message")
			{
				REQUIRE(store.from_id(id)==m2);
				AND_WHEN("the message is removed")
				{
					CHECK(store.remove(id)==m2);
					THEN("no message is retrieved")
					{
						REQUIRE(store.from_id(id)==nullptr);
					}
				}
			}
			// m1 already removed by the store
			delete m2;
		}
	}
	REQUIRE(CoAPMessage::messages()==0);

}

SCENARIO("multiple messages are stored in a list in the order they are added, most recent first")
{
	const message_id_t id1 = 456;
	const message_id_t id2 = 345;
	REQUIRE(CoAPMessage::messages()==0);
	GIVEN("an empty message store")
	{
		CoAPMessageStore store;
		CoAPMessage* m1 = new CoAPMessage(id1);
		CoAPMessage* m2 = new CoAPMessage(id2);
		REQUIRE(store.from_id(id1)==nullptr);
		REQUIRE(store.from_id(id2)==nullptr);

		WHEN("adding two messages with distinct IDs")
		{
			REQUIRE(m1->get_id()==id1);
			REQUIRE(m2->get_id()==id2);
			REQUIRE(store.add(m1)==NO_ERROR);
			REQUIRE(store.add(m2)==NO_ERROR);

			THEN("the second message points to the first")
			{
				REQUIRE(m2->get_next()==m1);
				AND_THEN("both messages can be retrieved")
				{
					REQUIRE(store.from_id(id1)==m1);
					REQUIRE(store.from_id(id2)==m2);
				}
				AND_WHEN("removing the most recently added message")
				{
					REQUIRE(store.remove(id2)==m2);
					THEN("only that message is removed")
					{
						CHECK(m2->get_next()==nullptr);
						CHECK(store.from_id(id2)==nullptr);
						CHECK(store.from_id(id1)==m1);
					}
					delete m2;		// m1 will be removed by the store
				}
			}
		}
	}
	REQUIRE(CoAPMessage::messages()==0);
}

SCENARIO("adding the same message twice")
{
	const message_id_t id = 456;
	GIVEN("an empty message store")
	{
		Mock<MessageChannel> mock;
		CoAPMessageStore store;
		REQUIRE(store.from_id(id)==nullptr);
		CoAPMessage* m = new CoAPMessage(id);
		WHEN("the message is added twice")
		{
			CHECK(store.add(m)==NO_ERROR);
			THEN("the second add succeeds")
			{
				REQUIRE(store.add(m)==NO_ERROR);
			}
		}
	}
	REQUIRE(CoAPMessage::messages()==0);
}

void build_message_channel_mock(Mock<MessageChannel>& mock)
{
	When(Method(mock,notify_established)).AlwaysReturn(NO_ERROR);
	When(Method(mock,is_unreliable)).AlwaysReturn(false);
	When(Method(mock,command)).AlwaysReturn(NO_ERROR);

}

SCENARIO("an unacknowledged message is resent up to MAX_TRANSMIT times, with exponentially increasing delays, after which it is removed")
{
	REQUIRE(CoAPMessage::messages()==0);
	GIVEN("a channel and a confirmable message")
	{
		Mock<MessageChannel> mock;
		MessageChannel& channel = mock.get();
		uint8_t buf[10];
		When(Method(mock,create)).AlwaysDo([&buf](Message& msg, size_t len)
			{
				msg.set_buffer(buf, 10); return NO_ERROR;
			});
		build_message_channel_mock(mock);

		Message m;
		channel.create(m, 5);
		uint8_t data[] = { 0x40, 0, 0x12, 0x34 };
		memcpy(m.buf(), data, 4);
		m.set_length(4);
		m.decode_id();
		message_id_t id = m.get_id();
		CoAPMessageStore store;
		REQUIRE(id==0x1234);
		REQUIRE(store.is_confirmable(m.buf()));

		WHEN("the message is sent")
		{
			REQUIRE(store.send(m, 0)==NO_ERROR);
			THEN("the message is pending")
			{
				CoAPMessage* cm = store.from_id(id);
				REQUIRE(cm!=nullptr);
				REQUIRE(cm->get_timeout()<CoAPMessage::ACK_TIMEOUT*CoAPMessage::ACK_RANDOM_FACTOR/CoAPMessage::ACK_RANDOM_DIVISOR);
				REQUIRE(cm->get_timeout()>=CoAPMessage::ACK_TIMEOUT*1);

				// the send() function should be invoked with a message corresponding to the coap message
				auto verify_message = [cm](Message& msg)->ProtocolError {
					REQUIRE(msg.get_id()==cm->get_id());
					REQUIRE(msg.length()==cm->get_data_length());
					REQUIRE(!memcmp(msg.buf(), cm->get_data(), msg.length()));
					return NO_ERROR;
				};
				When(Method(mock,send)).AlwaysDo(verify_message);

				AND_WHEN("the message times out MAX_RETRANSMIT times")
				{
					system_tick_t last_timeout = 0;
					for (int i=0; i<CoAPMessage::MAX_RETRANSMIT; i++)
					{
						CoAPMessage* m = store.from_id(id);
						REQUIRE(m->get_timeout()>last_timeout);
						last_timeout = m->get_timeout();
						store.process(m->get_timeout(), channel);
						Verify(Method(mock,send)).Exactly(i+1);
						REQUIRE(store.from_id(id)==m);
					}

					THEN("the message has been sent MAX_RETRANSMIT times")
					{
						Verify(Method(mock,send)).Exactly(CoAPMessage::MAX_RETRANSMIT);

						AND_WHEN("process is invoked once more")
						{
							// the final time removes it from the store
							store.process(cm->get_timeout(), channel);

							AND_THEN("send is not called again and the message the message is no longer pending")
							{
								Verify(Method(mock,send)).Exactly(CoAPMessage::MAX_RETRANSMIT);
								REQUIRE(store.from_id(id)==nullptr);
							}
						}
					}
				}

				AND_WHEN("the message times out 3 times")
				{
					system_tick_t last_timeout = 0;
					for (int i=0; i<3; i++)
					{
						CoAPMessage* m = store.from_id(id);
						REQUIRE(m->get_timeout()>=last_timeout);
						last_timeout = m->get_timeout();
						store.process(m->get_timeout(), channel);
						Verify(Method(mock,send)).Exactly(i+1);
						REQUIRE(store.from_id(id)==m);
					}

					THEN("the message has been sent 3 times")
					{
						Verify(Method(mock,send)).Exactly(3);
						AND_WHEN("the message is acknowledged")
						{
							system_tick_t timeout = cm->get_timeout();
							m.set_length(Messages::empty_ack(m.buf(), 0x12, 0x34));
							store.receive(m, channel, 0);

							THEN("the message is no longer pending")
							{
								REQUIRE(store.from_id(id)==nullptr);
								REQUIRE(CoAPMessage::messages()==0);

								// the final time removes it from the store
								store.process(timeout, channel);
								Verify(Method(mock,send)).Exactly(3);
								REQUIRE(store.from_id(id)==nullptr);
							}
						}
					}
				}

			}
		}
	}
	REQUIRE(CoAPMessage::messages()==0);

}

SCENARIO("a CoAPMessage can be created with the message buffer part of the allocation")
{
	// todo - factor out the message tests to their own test suite
	Message msg;
	msg.set_id(1234);
	uint8_t buf[234];
	msg.set_buffer(buf, sizeof(buf));

	REQUIRE(msg.has_id());
	REQUIRE(msg.get_id()==1234);

	REQUIRE(msg.length()==0);
	REQUIRE(msg.capacity()==sizeof(buf));

	for (int i=0; i<sizeof(buf); i++)
		buf[i] = rand();

	msg.set_length(sizeof(buf));
	REQUIRE(msg.length()==sizeof(buf));

	CoAPMessage* coapmsg = CoAPMessage::create(msg);
	REQUIRE(coapmsg!=nullptr);

	REQUIRE(coapmsg->get_id()==1234);
	REQUIRE(coapmsg->get_next()==nullptr);
	REQUIRE(coapmsg->matches(1234));
	REQUIRE(coapmsg->get_data_length()==sizeof(buf));

	const uint8_t* data = coapmsg->get_data();
	REQUIRE(data!=nullptr);

	for (int i=0; i<sizeof(buf); i++) {
		INFO( "checking coapmsg buffer index " << i);
		if (data[i]!=buf[i])
			FAIL("buffer content is different");
	}
	delete coapmsg;
	REQUIRE(CoAPMessage::messages()==0);
}

SCENARIO("a CoAPMessage can be created with a smaller buffer")
{
	GIVEN("a message")
	{
		uint8_t buf[] = { 0x40, 0x00, 0x12, 0x34, 0xFF, 1, 2, 3, 4, 0, 0, 0 };	// 12 bytes, 9 of them message content
		Message m(buf, 12, 9);
		m.decode_id();

		WHEN("A CoAP message of length 5 is created")
		{
			CoAPMessage* cm = CoAPMessage::create(m, 5);
			THEN("It has a distinct buffer of size 5")
			{
				REQUIRE(cm!=nullptr);
				REQUIRE(cm->get_data()!=buf);
				REQUIRE(cm->get_data_length()==5);
			}
			THEN("that equals the first 5 bytes of the orginal buffer")
			{
				REQUIRE(!memcmp(cm->get_data(), buf, 5));
			}
			delete cm;
		}
	}

}

SCENARIO("sending a non-confirmable message does not add a new coap message to the store")
{
	GIVEN("a reliable channel")
	{
		Mock<MessageChannel> mock;
		MessageChannel& channel = mock.get();
		CoAPMessageStore store;
		uint8_t buf[10];
		When(Method(mock,create)).AlwaysDo([&buf](Message& msg, size_t len)
			{
				msg.set_buffer(buf, 10); return NO_ERROR;
			});
		When(Method(mock,send)).AlwaysReturn(NO_ERROR);

		Message m;
		channel.create(m, 5);
		uint8_t data[] = { 0x50, 0, 0x12, 0x34 };
		memcpy(m.buf(), data, 4);
		m.set_length(4);
		m.decode_id();
		REQUIRE(m.get_type()==CoAPType::NON);
		REQUIRE(m.get_id()==0x1234);
		WHEN("the message is sent")
		{
			store.send(m, 0);
			THEN("the message is sent without being stored")
			{
				REQUIRE(store.from_id(0x1234)==nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages()==0);
}

SCENARIO("a Confirmable message is pending until acknowledged, reset or timeout")
{
	GIVEN("a channel and a confirmable message")
	{
		Mock<MessageChannel> mock;
		MessageChannel& channel = mock.get();
		uint8_t buf[10];
		When(Method(mock,create)).AlwaysDo([&buf](Message& msg, size_t len)
			{
				msg.set_buffer(buf, 10); return NO_ERROR;
			});
		When(Method(mock,send)).AlwaysReturn(NO_ERROR);
		build_message_channel_mock(mock);

		Message m;
		channel.create(m, 5);
		uint8_t data[] = { 0x40, 0, 0x12, 0x34 };
		memcpy(m.buf(), data, 4);
		m.set_length(4);
		m.decode_id();
		message_id_t id = m.get_id();
		REQUIRE(CoAPMessage::messages()==0);

		WHEN("the message is sent")
		{
			CoAPMessageStore store;
			REQUIRE(id==0x1234);
			REQUIRE(store.is_confirmable(m.buf()));

			REQUIRE(store.send(m, 0)==NO_ERROR);
			THEN("the message is pending")
			{
				REQUIRE(store.from_id(id)!=nullptr);
				AND_WHEN("the message is acknowledged")
				{
					m.set_length(Messages::empty_ack(m.buf(), 0x12, 0x34));
					store.receive(m, channel, 0);
					REQUIRE(m.length()>0);		// ack is available to application
					THEN("the message is no longer pending")
					{
						REQUIRE(store.from_id(id)==nullptr);
						REQUIRE(CoAPMessage::messages()==0);
					}
					AND_WHEN("the message is acknowledged a second time")
					{
						m.set_length(Messages::empty_ack(m.buf(), 0x12, 0x34));
						store.receive(m, channel, 0);

						THEN("the message is no longer pending")
						{
							REQUIRE(store.from_id(id)==nullptr);
							REQUIRE(CoAPMessage::messages()==0);
						}
						AND_THEN("the message is not made available to the application")
						{
							REQUIRE(m.length()==0);
						}
					}
				}

				AND_WHEN("the message is reset")
				{
					m.set_length(Messages::reset(m.buf(), 0x12, 0x34));
					store.receive(m, channel, 0);

					THEN("the message is no longer pending")
					{
						REQUIRE(store.from_id(id)==nullptr);
						REQUIRE(CoAPMessage::messages()==0);
					}
				}
			}
		}
	}
	REQUIRE(CoAPMessage::messages()==0);
}

using particle::protocol::time_has_passed;

SCENARIO("Rollover timestamps can be used to determine a timeout")
{
	REQUIRE(time_has_passed(2000,1000));
	REQUIRE(time_has_passed(2000,2000));
	REQUIRE(!time_has_passed(2000,3000));

	REQUIRE(time_has_passed(0x80000000, 1));
	REQUIRE(!time_has_passed(0x80000000, 0));

	REQUIRE(time_has_passed(1000, 0xFFFFFFFF));
	REQUIRE(!time_has_passed(0xFFFFFFFF, 1000));

	REQUIRE(time_has_passed(0, 0x80000000));
	REQUIRE(!time_has_passed(1, 0x80000000));

}

SCENARIO("retransmit delay is exponential with randomness")
{
	for (int i=0; i<CoAPMessage::MAX_RETRANSMIT; i++)
	{
		system_tick_t gmin = INT_MAX;
		system_tick_t gmax = 0;
		for (unsigned u = 0; u<500; u++)
		{
			system_tick_t timeout = CoAPMessage::transmit_timeout(i);
			system_tick_t min = 4000 << i;
			system_tick_t max = min * 1.5;
			if (timeout<min)
				REQUIRE(timeout>=min);
			if (timeout>max)
				REQUIRE(timeout<=max);
			if (timeout<gmin)
				gmin = timeout;
			if (timeout>gmax)
				gmax = timeout;
		}
		REQUIRE(gmin<gmax);

	}
}



SCENARIO("a repeated confirmable CoAP message is passed only once to the application and the acknowledgement is retained and returned until MAX_TRANSMIT_SPAN time has elapsed")
{
	GIVEN("a Confirmable message is received multiple times")
	{
		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));		// confirmable message
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& channel = mock.get();
		CoAPMessageStore store;

		WHEN("the Confirmable message is received first time")
		{
			REQUIRE(store.receive(m, channel, 0)==NO_ERROR);
			THEN("it is made available to the application")
			{
				REQUIRE(m.length()==9);						// message is available to application
				REQUIRE(store.from_id(0x1234)!=nullptr);		// todo - could compare message content to original message

				AND_WHEN("the Confirmable message is received again")
				{
					REQUIRE(store.receive(m, channel, 123)==NO_ERROR);
					THEN("then no response is sent to the channel")
					{
						AND_THEN("the message is not returned to the application")
						{
							REQUIRE(m.length()==0);
							AND_WHEN("MAX_TRANSMIT_SPAN time has elapsed the message is removed")
							{
								CoAPMessage* msg = store.from_id(0x1234);
								REQUIRE(msg!=nullptr);
								REQUIRE(msg->get_timeout()==45*1000);
								store.process(msg->get_timeout(), channel);
								REQUIRE(store.from_id(0x1234)==nullptr);
							}
						}
					}
				}

				AND_WHEN("the message is acknowledged and the message is received again")
				{
					uint8_t buf2[9];
					Message ack(buf2, sizeof(buf2), sizeof(buf2));
					ack.set_length(Messages::empty_ack(buf2, 0x12, 0x34));
					ack.decode_id();
					store.send(ack, 0);		// application response

					auto validate_message = [ack](Message& msg){
						REQUIRE(msg.get_id()==0x1234);
						REQUIRE(msg.length()==ack.length());
						REQUIRE(!memcmp(msg.buf(), ack.buf(), msg.length()));
						return NO_ERROR;
					};
					When(Method(mock,send)).Do(validate_message);
					REQUIRE(store.receive(m, channel,123)==NO_ERROR);
					THEN("the previous response is sent to the channel")
					{
						Verify(Method(mock,send)).Exactly(Once);
					}
					AND_THEN("the message is not returned to the application")
					{
						REQUIRE(m.length()==0);
						AND_WHEN("MAX_TRANSMIT_SPAN time has elapsed the message is removed")
						{
							CoAPMessage* msg = store.from_id(0x1234);
							REQUIRE(msg!=nullptr);
							REQUIRE(msg->get_timeout()==45*1000);
							store.process(msg->get_timeout(), channel);
							REQUIRE(store.from_id(0x1234)==nullptr);
						}
					}
				}
			}
		}
	}
}

template<typename R>
struct Callme
{
	virtual R operator()()=0;
};

SCENARIO("the send_synchronous method blocks until the message is sent or the send fails or the send times out")
{
	GIVEN("A confirmable message marked as confirm received")
	{
		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& channel = mock.get();
		CoAPMessageStore store;

		typedef Callme<system_tick_t> time_source;
		Mock<time_source> mockTime;
		time_source& time = mockTime.get();
		system_tick_t time_count = 0;
		auto time_increment = [&time_count]() { return time_count+=1000; };
		When(Method(mockTime, operator())).AlwaysDo(time_increment);

		WHEN("the message is sent")
		{
			REQUIRE(m.get_id()==0x1234);
			AND_WHEN("the channel retrieves a response")
			{
				uint8_t buf2[5];
				Message ack(buf2, sizeof(buf2));
				ack.set_length(Messages::empty_ack(buf2, 0x12, 0x34));

				auto receive_ack = [&ack](Message& msg) {
					msg.set_buffer(ack.buf(), ack.capacity());
					msg.set_length(ack.length());
					return NO_ERROR;
				};
				When(Method(mock,receive)).Do(receive_ack);

				auto verify_msg = [&m](Message& msg) {
					REQUIRE(m.get_id()==msg.get_id());
					REQUIRE(m.length()==msg.length());
					REQUIRE(!memcmp(m.buf(), msg.buf(), msg.length()));
					return NO_ERROR;
				};
				When(Method(mock,send)).AlwaysDo(verify_msg);

				THEN("the send_synchronous method returns success")
				{
					ProtocolError error = store.send_synchronous(m, channel, time);
					REQUIRE(error==NO_ERROR);
					AND_THEN("the CoAP message is purged")
					{
						REQUIRE(store.from_id(0x1234)==nullptr);
					}
				}
			}

			AND_WHEN("no response is received")
			{
				// receive endlessly receives no packets
				auto receive_none = [](Message& msg) {
					msg.set_length(0);
					return NO_ERROR;
				};
				When(Method(mock,receive)).AlwaysDo(receive_none);

				auto verify_msg = [buf](Message& msg) {
					// NB: the passed in message instance is re-used to retrieve messages from the channel
					// hence m will be equal to th eempty message. (Previous code tried comparing m==msg which consequently failed.)

					REQUIRE(9==msg.length());
					REQUIRE(!memcmp(buf, msg.buf(), msg.length()));
					REQUIRE(0x1234==msg.get_id());
					return NO_ERROR;
				};
				When(Method(mock,send)).AlwaysDo(verify_msg);

				THEN("the send_synchronous method times out")
				{
					ProtocolError error = store.send_synchronous(m, channel, time);
					REQUIRE(error==MESSAGE_TIMEOUT);
					AND_THEN("the CoAP message is purged")
					{
						REQUIRE(store.from_id(0x1234)==nullptr);
					}
				}
			}

			AND_WHEN("the error response IO_ERROR is received on the first send")
			{
				When(Method(mock,send)).Return(IO_ERROR);
				THEN("the send_synchronous method fails with IO_ERROR")
				{
					ProtocolError error = store.send_synchronous(m, channel, time);
					REQUIRE(error==IO_ERROR);
					AND_THEN("the CoAP message is purged")
					{
						REQUIRE(store.from_id(0x1234)==nullptr);
					}
				}
			}

			AND_WHEN("the error response IO_ERROR is received the second send")
			{
				When(Method(mock,send)).Return(NO_ERROR);
				When(Method(mock,send)).Return(IO_ERROR);
				THEN("the send_synchronous method fails with IO_ERROR")
				{
					ProtocolError error = store.send_synchronous(m, channel, time);
					REQUIRE(error==IO_ERROR);
					AND_THEN("the CoAP message is purged")
					{
						REQUIRE(store.from_id(0x1234)==nullptr);
					}
				}
			}
		}
	}
}

/**
 * A reliable CoAP Channel that uses a forwarding message channel.
 * This allows the actual message channel to be set later (e.g. a mock.)
 * The typename M is a callable that provides the current time.
 */
template <typename M>
class ForwardCoAPReliableChannel: public CoAPReliableChannel<ForwardMessageChannel, M>
{
	using super = CoAPReliableChannel<ForwardMessageChannel, M>;

public:

	ForwardCoAPReliableChannel(MessageChannel& ch, M m) : super(m)
	{
		this->setForward(&ch);
	}
};


SCENARIO("sending a message and re-establishing the connection clears existing messages")
{
	GIVEN("a CoAPReliableChannel")
	{
		Mock<MessageChannel> mock;
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };	// time not needed here
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		WHEN("a CON message is sent")
		{
			uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
			Message m(buf, sizeof(buf), sizeof(buf));
			m.decode_id();

			When(Method(mock, send)).Return(NO_ERROR);	// send on the channel is called once
			channel.send(m);								// send and register with the store

			THEN("the message is retained in the client store")
			{
				REQUIRE(channel.client_messages().from_id(0x1234)!=nullptr);		// message has been sent and registered
				REQUIRE(CoAPMessage::messages()==1);
				Verify(Method(mock,send)).Exactly(Once);
			}
			AND_WHEN("the connection is re-established")
			{
				When(Method(mock,establish)).Return(NO_ERROR);
				uint32_t flags;
				channel.establish(flags, 0);
				THEN("the message store is cleared")
				{
					REQUIRE(channel.client_messages().from_id(0x1234)==nullptr);		// message has been sent and registered
					REQUIRE(CoAPMessage::messages()==0);
					Verify(Method(mock,establish)).Exactly(Once);
				}
			}
		}
	}
}


SCENARIO("sending a reliable message first passes it to the store, and then on to the channel")
{
	GIVEN("a CoAPReliableChannel")
	{
		Mock<MessageChannel> mock;
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };	// time not needed here
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		WHEN("an invalid message is sent")
		{
			ProtocolError result = channel.send(m);
			THEN("the message is not sent to the channel")
			{
				Verify(Method(mock,send)).Exactly(0);
				REQUIRE(result==MISSING_MESSAGE_ID);
				REQUIRE(channel.client_messages().from_id(0x1234)==nullptr);
				REQUIRE(channel.server_messages().from_id(0x1234)==nullptr);
			}
		}

		AND_WHEN("a valid message is sent")
		{
			m.decode_id();	// set up the ID
			When(Method(mock,send)).Return(NO_ERROR);
			ProtocolError result = channel.send(m);
			THEN("the message is sent to the channel")
			{
				Verify(Method(mock,send)).Exactly(1);
				REQUIRE(result==NO_ERROR);
				REQUIRE(channel.client_messages().from_id(0x1234)!=nullptr);
				REQUIRE(channel.server_messages().from_id(0x1234)==nullptr);
			}
		}

		AND_WHEN("a valid message is sent but fails sending")
		{
			m.decode_id();	// set up the ID
			When(Method(mock,send)).Return(IO_ERROR);
			ProtocolError result = channel.send(m);
			THEN("the message is remains in the store and the send error is returned")
			{
				Verify(Method(mock,send)).Exactly(1);
				REQUIRE(result==IO_ERROR);
				// still maintained in the client store.
				REQUIRE(channel.client_messages().from_id(0x1234)!=nullptr);
				REQUIRE(channel.server_messages().from_id(0x1234)==nullptr);
			}
		}
	}
}


SCENARIO("receiving a message first retrieves from the channel and then passes the message to the store")
{
	GIVEN("a CoAPReliableChannel")
	{
		Mock<MessageChannel> mock;
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(234); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t con[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		uint8_t buf[9];
		Message m(buf, sizeof(buf));		// empty message
		m.decode_id();

		WHEN("the message is received")
		{
			auto receive_con = [con](Message& msg) {
				memcpy(msg.buf(), con, 9);
				msg.set_length(9);
				return NO_ERROR;
			};
			When(Method(mock,receive)).Do(receive_con);
			ProtocolError result = channel.receive(m);
			THEN("it is passed to the server store")
			{
				Verify(Method(mock,receive)).Exactly(1);
				CoAPMessage* cm = channel.server_messages().from_id(0x1234);
				REQUIRE(cm!=nullptr);
				REQUIRE(cm->get_timeout()==CoAPMessage::MAX_TRANSMIT_SPAN + 234);
			}
		}
	}
}

bool message_equals(CoAPMessage* cm, Message& msg, const char* name)
{
	INFO("checking message " << name);
	REQUIRE(cm!=nullptr);
	REQUIRE(cm->get_id()==msg.get_id());

	REQUIRE(	cm->get_data_length()==msg.length());
	REQUIRE(!memcmp(cm->get_data(), msg.buf(), msg.length()));
	return true;
}

SCENARIO("client and server can send requests with the same message ID and they are not confused")
{
	GIVEN("a reliable CoAP channel")
	{
		Mock<MessageChannel> mock;
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(234); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t con_server[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		uint8_t con_server_trunc[] = { 0x40, 0, 0x12, 0x34, 0xFF };		// truncated to 5
		uint8_t con_client[] = { 0x40, 0, 0x12, 0x34, 0xFF, 5 };
		Message client(con_client, sizeof(con_client), sizeof(con_client));
		Message server(con_server, sizeof(con_server), sizeof(con_server));
		client.decode_id();
		server.decode_id();
		REQUIRE(client.get_id()==server.get_id());

		WHEN("the client and server each sends a Confirmable message")
		{
			// send the client message
			When(Method(mock,send)).Return(NO_ERROR);
			REQUIRE(channel.send(client)==NO_ERROR);
			Verify(Method(mock,send)).Exactly(Once);

			// receive the server message
			auto receive_server_con = [con_server](Message& msg) {
				memcpy(msg.buf(), con_server, sizeof(con_server));
				msg.set_length(sizeof(con_server));
				return NO_ERROR;
			};
			When(Method(mock,receive)).Do(receive_server_con);
			uint8_t buf[20];				// holding area for the received message
			Message m(buf, 20);
			REQUIRE(channel.receive(m)==NO_ERROR);
			REQUIRE(m.length()==sizeof(con_server));			// message is received in full
			Verify(Method(mock,receive)).Exactly(Once);
			mock.Reset();

			REQUIRE(message_equals(channel.client_messages().from_id(0x1234), client, "client"));

			// the server isn't stored verbatim, but truncated to 5 bytes
			m.copy(con_server_trunc, sizeof(con_server_trunc));
			REQUIRE(message_equals(channel.server_messages().from_id(0x1234), m, "server"));

			THEN("the messages are separately acknowledged")
			{
				// ACK message from server
				uint8_t buf_ack[5];
				Message ack(buf_ack, 5);
				ack.set_length(Messages::empty_ack(buf_ack, 0x12, 0x34));

				// receive ACK from server for client message
				auto receive_server_ack = [](Message& msg) {
					msg.set_length(Messages::empty_ack(msg.buf(), 0x12, 0x34));
					return NO_ERROR;
				};
				When(Method(mock,receive)).Do(receive_server_ack);
				REQUIRE(channel.receive(m)==NO_ERROR);
				Verify(Method(mock,receive)).Exactly(Once);
				// ACK message returned to application
				REQUIRE(ack.content_equals(m));
				// message has been acked and is removed
				REQUIRE(channel.client_messages().from_id(0x1234)==nullptr);

				// server CON message is unchanged
				CoAPMessage* cm = channel.server_messages().from_id(0x1234);
				REQUIRE(cm!=nullptr);
				REQUIRE(cm->is_request());

				// send ACK from client
				mock.Reset();
				ack.decode_id();		// ensure the ID is set
				When(Method(mock,send)).Return(NO_ERROR);
				REQUIRE(channel.send(ack)==NO_ERROR);
				Verify(Method(mock,send)).Exactly(Once);

				// acknowledgement is maintained to return to the server for repeated requests.
				cm = channel.server_messages().from_id(0x1234);
				REQUIRE(cm!=nullptr);
				REQUIRE(cm->is_request()==false);		// not a request, but a response
				// client messages unaffected
				REQUIRE(channel.client_messages().from_id(0x1234)==nullptr);
			}
		}
	}
}


SCENARIO("a message flagged as confirm received waits synchronously for acknowledgement")
{
	GIVEN("a message flagged as confirm received and a reliable channel")
	{
		Mock<MessageChannel> mock;
		MessageChannel& delegate = mock.get();
		system_tick_t ticks = 0;
		auto time = [&ticks]() { return ticks+=10; };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();

		// receive ACK from server for client message
		auto receive_server_ack = [](Message& msg) {
			msg.set_length(Messages::empty_ack(msg.buf(), 0x12, 0x34));
			return NO_ERROR;
		};

		auto no_message = [](Message& msg) {
			msg.set_length(0);
			return NO_ERROR;
		};

		WHEN("the message is sent and not acknowledged")
		{
			When(Method(mock,send)).AlwaysReturn(NO_ERROR);
			When(Method(mock,receive)).AlwaysDo(no_message);
			ProtocolError error = channel.send(m);
			AND_WHEN("the request has timed out")
			{
				CHECK(ticks>=CoAPMessage::MAX_TRANSMIT_SPAN+0);
			}
			THEN("an error response is returned")
			{
				REQUIRE(error==MESSAGE_TIMEOUT);
				Verify(Method(mock,send)).Exactly(CoAPMessage::MAX_RETRANSMIT+1);
			}
		}

		WHEN("the message is sent and negatively acknowledged")
		{
			When(Method(mock,send)).Return(NO_ERROR);
			auto receive_nak = [](Message& msg) {
				msg.set_length(Messages::reset(msg.buf(), 0x12, 0x34));
				return NO_ERROR;
			};
			When(Method(mock,receive)).Do(receive_nak);
			ProtocolError error = channel.send(m);
			THEN("a MESSAGE_RESET response is returned")
			{
				REQUIRE(error==MESSAGE_RESET);
				REQUIRE(ticks<=50);	// was 30 when tested - not critical
			}
		}

		WHEN("the message is sent and acknowledged")
		{
			When(Method(mock,send)).AlwaysReturn(NO_ERROR);
			When(Method(mock,receive)).Do(receive_server_ack);
			ProtocolError error = channel.send(m);
			THEN("a success response is returned")
			{
				REQUIRE(error==NO_ERROR);
				REQUIRE(ticks<=50);	// was 30 when tested - not critical
			}
		}

	}
}
