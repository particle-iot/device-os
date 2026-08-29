/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
 *
 * Unit tests for the non-blocking DTLS handshake changes:
 *   - send_confirmed / poll_confirmed split (CoAPReliableChannel)
 *   - client_next_timeout() wrap-safe arithmetic + expired floor
 *   - Protocol::begin() stage machine (ESTABLISH → HELLO_WAIT → FINALIZE)
 */

#include <climits>

#include "coap_channel.h"
#include "forward_message_channel.h"
#include "messages.h"
#include "protocol.h"
#include "util/protocol_stub.h"

#include <catch2/catch.hpp>
#include "fakeit.hpp"

using namespace particle::protocol;
using namespace fakeit;

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

static void build_message_channel_mock(Mock<MessageChannel>& mock)
{
	When(Method(mock,notify_established)).AlwaysReturn(NO_ERROR);
	When(Method(mock,is_unreliable)).AlwaysReturn(false);
	When(Method(mock,command)).AlwaysReturn(NO_ERROR);
	When(Method(mock,notify_client_messages_processed)).AlwaysReturn();
	// poll_confirmed() calls create() to get a receive buffer
	When(Method(mock,create)).AlwaysDo([](Message& msg, size_t) {
		static uint8_t buf[1500];
		msg.set_buffer(buf, sizeof(buf));
		return NO_ERROR;
	});
	// establish() calls reset() which calls channel::reset()
	When(Method(mock,reset)).AlwaysReturn();
	// establish() calls is_establish_in_progress() to check continuation
	When(Method(mock,is_establish_in_progress)).AlwaysReturn(false);
}

SCENARIO("send_confirmed sends a CON message and returns NO_ERROR without waiting for ACK",
         "[nonblocking_handshake]")
{
	GIVEN("a reliable channel and a confirmable message marked confirm_received")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();

		When(Method(mock, send)).AlwaysReturn(NO_ERROR);

		WHEN("send_confirmed is called")
		{
			ProtocolError error = channel.send_confirmed(m);
			THEN("it returns NO_ERROR immediately")
			{
				REQUIRE(error == NO_ERROR);
			}
			THEN("the message is in the client store (unacknowledged)")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) != nullptr);
			}
			THEN("send was called exactly once (the initial send)")
			{
				Verify(Method(mock, send)).Exactly(1);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("poll_confirmed returns IN_PROGRESS when ACK has not arrived",
         "[nonblocking_handshake]")
{
	GIVEN("a message sent via send_confirmed, no ACK received")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send_confirmed(m);

		WHEN("poll_confirmed is called and no data is received")
		{
			auto receive_none = [](Message& msg) {
				msg.set_length(0);
				return NO_ERROR;
			};
			When(Method(mock, receive)).AlwaysDo(receive_none);

			ProtocolError error = channel.poll_confirmed();
			THEN("it returns IN_PROGRESS")
			{
				REQUIRE(error == IN_PROGRESS);
			}
			THEN("the message is still in the client store")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) != nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("poll_confirmed returns NO_ERROR when ACK arrives",
         "[nonblocking_handshake]")
{
	GIVEN("a message sent via send_confirmed")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send_confirmed(m);

		WHEN("an ACK for the message is received")
		{
			uint8_t ackbuf[5];
			auto receive_ack = [&ackbuf](Message& msg) {
				msg.set_buffer(ackbuf, sizeof(ackbuf));
				msg.set_length(Messages::empty_ack(ackbuf, 0x12, 0x34));
				return NO_ERROR;
			};
			When(Method(mock, receive)).Do(receive_ack);

			ProtocolError error = channel.poll_confirmed();
			THEN("it returns NO_ERROR")
			{
				REQUIRE(error == NO_ERROR);
			}
			THEN("the message is purged from the client store")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) == nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("poll_confirmed returns MESSAGE_RESET when RST arrives",
         "[nonblocking_handshake]")
{
	GIVEN("a message sent via send_confirmed")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send_confirmed(m);

		WHEN("a RST for the message is received")
		{
			uint8_t rstbuf[5];
			auto receive_rst = [&rstbuf](Message& msg) {
				msg.set_buffer(rstbuf, sizeof(rstbuf));
				msg.set_length(Messages::reset(rstbuf, 0x12, 0x34));
				return NO_ERROR;
			};
			When(Method(mock, receive)).Do(receive_rst);

			ProtocolError error = channel.poll_confirmed();
			THEN("it returns MESSAGE_RESET")
			{
				REQUIRE(error == MESSAGE_RESET);
			}
			THEN("the message is purged")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) == nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("poll_confirmed returns MESSAGE_TIMEOUT after MAX_RETRANSMIT retransmits",
         "[nonblocking_handshake]")
{
	GIVEN("a message sent via send_confirmed, with fake-millis advancing time")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		system_tick_t ticks = 0;
		auto time = [&ticks]() { return ticks += 1000; };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		auto receive_none = [](Message& msg) {
			msg.set_length(0);
			return NO_ERROR;
		};
		When(Method(mock, receive)).AlwaysDo(receive_none);
		channel.send_confirmed(m);

		WHEN("poll_confirmed is called repeatedly until timeout")
		{
			ProtocolError error = NO_ERROR;
			// Loop enough times for the time source to advance past
			// MAX_TRANSMIT_SPAN. Each poll_confirmed() call advances time
			// by ~1000ms per millis() call, and poll_confirmed calls
			// millis() several times, so ~100 iterations is plenty.
			for (int i = 0; i < 100; ++i) {
				error = channel.poll_confirmed();
				if (error != IN_PROGRESS) {
					break;
				}
			}
			THEN("it eventually returns MESSAGE_TIMEOUT")
			{
				REQUIRE(error == MESSAGE_TIMEOUT);
			}
			THEN("the message is purged")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) == nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("send_synchronous wrapper still works (legacy compatibility)",
         "[nonblocking_handshake]")
{
	GIVEN("a confirmable message marked confirm_received")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.set_confirm_received(true);
		m.decode_id();

		WHEN("the channel retrieves an ACK response")
		{
			uint8_t ackbuf[5];
			auto receive_ack = [&ackbuf](Message& msg) {
				msg.set_buffer(ackbuf, sizeof(ackbuf));
				msg.set_length(Messages::empty_ack(ackbuf, 0x12, 0x34));
				return NO_ERROR;
			};
			When(Method(mock, send)).AlwaysReturn(NO_ERROR);
			When(Method(mock, receive)).Do(receive_ack);

			THEN("send_synchronous returns NO_ERROR")
			{
				REQUIRE(channel.send_synchronous(m) == NO_ERROR);
				REQUIRE(channel.client_messages().from_id(0x1234) == nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("client_next_timeout returns 0 when no messages are pending",
         "[nonblocking_handshake]")
{
	GIVEN("a reliable channel with no pending messages")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		THEN("client_next_timeout returns 0")
		{
			REQUIRE(channel.client_next_timeout(1000) == 0);
		}
	}
}

SCENARIO("client_next_timeout returns remaining time for a pending message",
         "[nonblocking_handshake]")
{
	GIVEN("a channel with a sent CON message")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		system_tick_t ticks = 0;
		auto time = [&ticks]() { return ticks += 100; };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send(m); // ticks is now 100 after one time() call

		WHEN("queried at a time before the timeout")
		{
			system_tick_t timeout = channel.client_next_timeout(200);
			THEN("it returns a positive remaining time")
			{
				REQUIRE(timeout > 0);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("client_next_timeout returns 1ms floor for an expired message",
         "[nonblocking_handshake]")
{
	GIVEN("a channel with a sent CON message")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		system_tick_t ticks = 0;
		auto time = [&ticks]() { return ticks += 100; };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send(m); // ticks = 100

		WHEN("queried at a time far in the future (expired)")
		{
			// The message timeout is around ACK_TIMEOUT (4000ms) from send time.
			// Query at 10000ms - well past the deadline.
			system_tick_t timeout = channel.client_next_timeout(10000);
			THEN("it returns 1 (the expired floor, not 0)")
			{
				REQUIRE(timeout == 1);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("client_next_timeout is wrap-safe across 32-bit tick boundary",
         "[nonblocking_handshake]")
{
	GIVEN("a channel with a sent CON message whose timeout wraps")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		// Use a time source near the wrap boundary
		system_tick_t ticks = 0xFFFFFFF0;
		auto time = [&ticks]() { return ticks += 100; };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send(m); // ticks wraps to ~0x50

		WHEN("queried at a time after the wrap")
		{
			// The timeout should be a small positive number (remaining time
			// after the wrap), not a huge number from unsigned underflow
			system_tick_t timeout = channel.client_next_timeout(0x60);
			THEN("it returns a reasonable positive value (not a wrap artifact)")
			{
				// Should be > 0 and < ACK_TIMEOUT * ACK_RANDOM_FACTOR
				REQUIRE(timeout > 0);
				REQUIRE(timeout < ACK_TIMEOUT * ACK_RANDOM_FACTOR);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

// A ForwardMessageChannel that can fake is_establish_in_progress()
class TestForwardChannel : public ForwardMessageChannel {
	bool establishInProgress = false;
public:
	TestForwardChannel() : ForwardMessageChannel() {}
	void setEstablishInProgress(bool v) { establishInProgress = v; }
	bool is_establish_in_progress() const override { return establishInProgress; }
	ProtocolError establish() override { return NO_ERROR; }
};

SCENARIO("CoAPReliableChannel::establish() clears message stores on fresh attempt",
         "[nonblocking_handshake]")
{
	GIVEN("a reliable channel with a pending message, establish NOT in progress")
	{
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(delegate, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		channel.send(m);
		REQUIRE(channel.client_messages().from_id(0x1234) != nullptr);

		WHEN("establish() is called (fresh attempt, is_establish_in_progress=false)")
		{
			When(Method(mock, establish)).Return(NO_ERROR);
			channel.establish();

			THEN("the message store is cleared")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) == nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

SCENARIO("CoAPReliableChannel::establish() preserves message stores when establish is in progress",
         "[nonblocking_handshake]")
{
	GIVEN("a reliable channel with a pending message, establish IN progress")
	{
		// TestForwardChannel needs a forward target for send_confirmed()
		Mock<MessageChannel> mock;
		build_message_channel_mock(mock);
		MessageChannel& delegate = mock.get();

		TestForwardChannel forward;
		forward.setForward(&delegate);
		forward.setEstablishInProgress(true);
		auto time = []() { return system_tick_t(0); };
		ForwardCoAPReliableChannel<decltype(time)> channel(forward, time);

		uint8_t buf[] = { 0x40, 0, 0x12, 0x34, 0xFF, 1, 2, 3, 4 };
		Message m(buf, sizeof(buf), sizeof(buf));
		m.decode_id();
		When(Method(mock, send)).AlwaysReturn(NO_ERROR);
		// Send directly via send_confirmed to populate the store
		channel.send_confirmed(m);
		REQUIRE(channel.client_messages().from_id(0x1234) != nullptr);

		WHEN("establish() is called (continuation, is_establish_in_progress=true)")
		{
			channel.establish();

			THEN("the message store is preserved (not reset)")
			{
				REQUIRE(channel.client_messages().from_id(0x1234) != nullptr);
			}
		}
	}
	REQUIRE(CoAPMessage::messages() == 0);
}

// A CoapMessageChannel that can return IN_PROGRESS from establish()
class SteppableCoapChannel : public test::CoapMessageChannel {
	ProtocolError establishResult_ = NO_ERROR;
	bool establishInProgress = false;
public:
	unsigned establishCallCount = 0;
	unsigned moveSessionCallCount = 0;
	unsigned sendCallCount = 0;
	void setEstablishResult(ProtocolError e) { establishResult_ = e; }
	void setEstablishInProgress(bool v) { establishInProgress = v; }
	ProtocolError establish() override {
		establishCallCount++;
		return establishResult_;
	}
	ProtocolError command(Command cmd, void* arg) override {
		if (cmd == MOVE_SESSION) {
			moveSessionCallCount++;
		}
		return test::CoapMessageChannel::command(cmd, arg);
	}
	ProtocolError send(Message& msg) override {
		sendCallCount++;
		return test::CoapMessageChannel::send(msg);
	}
	bool is_establish_in_progress() const override {
		return establishInProgress;
	}
};

SCENARIO("begin() with NON_BLOCKING returns IN_PROGRESS when establish is IN_PROGRESS",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol with a channel that returns IN_PROGRESS from establish()")
	{
		SteppableCoapChannel channel;
		channel.setEstablishResult(IN_PROGRESS);
		channel.setEstablishInProgress(true);
		test::ProtocolStub p(&channel);

		WHEN("begin() is called with NON_BLOCKING flag")
		{
			system_tick_t delay = 0;
			int ret = p.begin(Protocol::HANDSHAKE_FLAG_NON_BLOCKING, &delay);

			THEN("it returns IN_PROGRESS")
			{
				REQUIRE(ret == IN_PROGRESS);
			}
		}
	}
}

SCENARIO("begin() CONTINUE without NON_BLOCKING is INVALID_STATE",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol")
	{
		SteppableCoapChannel channel;
		test::ProtocolStub p(&channel);

		WHEN("begin() is called with CONTINUE but not NON_BLOCKING")
		{
			int ret = p.begin(Protocol::HANDSHAKE_FLAG_CONTINUE, nullptr);

			THEN("it returns INVALID_STATE")
			{
				REQUIRE(ret == INVALID_STATE);
			}
		}
	}
}

SCENARIO("begin() CONTINUE at INIT is INVALID_STATE",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol with no prior handshake in progress")
	{
		SteppableCoapChannel channel;
		test::ProtocolStub p(&channel);

		WHEN("begin() is called with CONTINUE + NON_BLOCKING but no prior attempt")
		{
			int ret = p.begin(
				Protocol::HANDSHAKE_FLAG_NON_BLOCKING | Protocol::HANDSHAKE_FLAG_CONTINUE,
				nullptr);

			THEN("it returns INVALID_STATE")
			{
				REQUIRE(ret == INVALID_STATE);
			}
		}
	}
}

SCENARIO("begin() rejects unknown flag bits",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol")
	{
		SteppableCoapChannel channel;
		test::ProtocolStub p(&channel);

		WHEN("begin() is called with an unknown flag bit")
		{
			int ret = p.begin(0x80, nullptr); // bit 7 is not a valid flag

			THEN("it returns INVALID_STATE")
			{
				REQUIRE(ret == INVALID_STATE);
			}
		}
	}
}

SCENARIO("begin() zeroes next_event_delay on terminal result",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol with a channel that resumes session (skips hello)")
	{
		SteppableCoapChannel channel;
		channel.setEstablishResult(SESSION_RESUMED);
		test::ProtocolStub p(&channel);

		WHEN("begin() is called with NON_BLOCKING and session resumes")
		{
			system_tick_t delay = 999; // non-zero initial value
			int ret = p.begin(Protocol::HANDSHAKE_FLAG_NON_BLOCKING, &delay);

			THEN("next_event_delay is zeroed on terminal result")
			{
				REQUIRE(delay == 0);
			}
		}
	}
}

SCENARIO("begin() preserves the session-resumption fast path",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol whose channel restores a matching session")
	{
		SteppableCoapChannel channel;
		channel.setEstablishResult(SESSION_RESUMED);
		test::ProtocolStub p(&channel);

		WHEN("a non-blocking handshake is started")
		{
			system_tick_t delay = 999;
			const int ret = p.begin(Protocol::HANDSHAKE_FLAG_NON_BLOCKING, &delay);

			THEN("the session is moved and the resume ping is sent in the same call")
			{
				REQUIRE(ret == SESSION_RESUMED);
				REQUIRE(delay == 0);
				REQUIRE(channel.establishCallCount == 1);
				REQUIRE(channel.moveSessionCallCount == 1);
				REQUIRE(channel.sendCallCount == 1);
			}
		}
	}
}

SCENARIO("begin() CONTINUE re-enters establish() without fresh reset",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol where the first begin() returns IN_PROGRESS")
	{
		SteppableCoapChannel channel;
		channel.setEstablishResult(IN_PROGRESS);
		channel.setEstablishInProgress(true);
		test::ProtocolStub p(&channel);

		// First call: fresh attempt → establish() called once
		system_tick_t delay = 0;
		int ret1 = p.begin(Protocol::HANDSHAKE_FLAG_NON_BLOCKING, &delay);
		REQUIRE(ret1 == IN_PROGRESS);
		REQUIRE(channel.establishCallCount == 1);

		WHEN("the channel is flipped to succeed and begin(CONTINUE) is called")
		{
			// Use SESSION_RESUMED to skip hello() (ProtocolStub::build_hello
			// returns 0, which would crash channel.send())
			channel.setEstablishResult(SESSION_RESUMED);
			channel.setEstablishInProgress(false);
			int ret2 = p.begin(
				Protocol::HANDSHAKE_FLAG_NON_BLOCKING | Protocol::HANDSHAKE_FLAG_CONTINUE,
				&delay);

			THEN("establish() is called again (continuation, not fresh reset)")
			{
				REQUIRE(channel.establishCallCount == 2);
				// SESSION_RESUMED skips hello and returns SESSION_RESUMED
				REQUIRE(ret2 == SESSION_RESUMED);
			}
		}
	}
}

SCENARIO("begin() fresh restart after terminal error",
         "[nonblocking_handshake]")
{
	GIVEN("a Protocol where the first begin() returns an establish error")
	{
		SteppableCoapChannel channel;
		channel.setEstablishResult(IO_ERROR_GENERIC_ESTABLISH);
		test::ProtocolStub p(&channel);

		// First call: establish fails → terminal error
		int ret1 = p.begin(Protocol::HANDSHAKE_FLAG_NON_BLOCKING, nullptr);
		REQUIRE(ret1 != NO_ERROR);
		REQUIRE(ret1 != IN_PROGRESS);
		REQUIRE(channel.establishCallCount == 1);

		WHEN("a second begin() is called without CONTINUE")
		{
			// Use SESSION_RESUMED to skip hello() (ProtocolStub::build_hello
			// returns 0, which would crash channel.send())
			channel.setEstablishResult(SESSION_RESUMED);
			int ret2 = p.begin(Protocol::HANDSHAKE_FLAG_NON_BLOCKING, nullptr);

			THEN("it's a fresh attempt (establish() called again from scratch)")
			{
				REQUIRE(channel.establishCallCount == 2);
				REQUIRE(ret2 == SESSION_RESUMED);
			}
		}
	}
}
