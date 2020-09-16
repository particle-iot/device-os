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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "logging.h"

LOG_SOURCE_CATEGORY("comm.protocol")

#include "protocol.h"
#include "chunked_transfer.h"
#include "subscriptions.h"
#include "functions.h"

namespace particle { namespace protocol {

namespace {

enum HelloFlag {
	HELLO_FLAG_OTA_UPGRADE_SUCCESSFUL = 0x01,
	HELLO_FLAG_DIAGNOSTICS_SUPPORT = 0x02,
	HELLO_FLAG_IMMEDIATE_UPDATES_SUPPORT = 0x04,
	// Flag 0x08 is reserved to indicate support for the HandshakeComplete message
	HELLO_FLAG_GOODBYE_SUPPORT = 0x10,
	HELLO_FLAG_DEVICE_INITIATED_DESCRIBE = 0x20,
	HELLO_FLAG_COMPRESSED_OTA = 0x40,
	HELLO_FLAG_OTA_PROTOCOL_V3 = 0x80
};

} // namespace

/**
 * Sends an empty acknowledgement for the given message
 */
ProtocolError Protocol::send_empty_ack(Message& message, message_id_t msg_id)
{
	message.set_length(Messages::empty_ack(message.buf(), 0, 0));
	message.set_id(msg_id);
	return channel.send(message);
}

/**
 * Decodes and dispatches a received message to its handler.
 */
ProtocolError Protocol::handle_received_message(Message& message,
		CoAPMessageType::Enum& message_type)
{
	last_message_millis = callbacks.millis();
	pinger.message_received();
	uint8_t* queue = message.buf();
	message_type = Messages::decodeType(queue, message.length());
	// todo - not all requests/responses have tokens. These device requests do not use tokens:
	// Update Done, ChunkMissed, event, ping, hello
	token_t token = 0;
	size_t token_len = CoAP::token(queue, &token);
	if (token_len > 0 && token_len != sizeof(token_t)) {
		LOG(ERROR, "Unsupported token length: %u", (unsigned)token_len);
		token_len = 0;
	}
	message_id_t msg_id = CoAP::message_id(queue);
	CoAPCode::Enum code = CoAP::code(queue);
	CoAPType::Enum type = CoAP::type(queue);
	if (CoAPType::is_reply(type)) {
		LOG(TRACE, "Reply recieved: type=%d, code=%d", type, code);
		// todo - this is a little too simple in the case of an empty ACK for a separate response
		// the message should then be bound to the token. see CH19037
		if (type == CoAPType::RESET) { // RST is sent with an empty code. It's like an unspecified error
			LOG(TRACE, "Reset received, setting error code to internal server error.");
			code = CoAPCode::INTERNAL_SERVER_ERROR;
		}
		notify_message_complete(msg_id, code);
		handle_app_state_reply(msg_id, code);
#if HAL_PLATFORM_OTA_PROTOCOL_V3
		if (type == CoAPType::ACK && firmwareUpdate.isRunning()) {
			const ProtocolError error = firmwareUpdate.responseAck(&message);
			if (error != ProtocolError::NO_ERROR) {
				return error;
			}
		}
#endif
	}

	ProtocolError error = NO_ERROR;
	//LOG(WARN,"message type %d", message_type);
	// todo - would be good to separate requests from responses here.
	switch (message_type)
	{
	case CoAPMessageType::DESCRIBE:
	{
		// 4 bytes header, 1 byte token, 2 bytes Uri-Path
		// 2 bytes optional single character Uri-Query for describe flags
		int descriptor_type = DESCRIBE_DEFAULT;
		if (message.length() > 8 && queue[8] <= DESCRIBE_MAX) {
			descriptor_type = queue[8];
		} else if (message.length() > 8) {
			LOG(WARN, "Invalid DESCRIBE flags: 0x%02x", (unsigned)queue[8]);
		}
		LOG(INFO, "Received DESCRIBE request; flags: 0x%02x", (unsigned)descriptor_type);
		error = send_description_response(token, msg_id, descriptor_type);
		break;
	}

	case CoAPMessageType::FUNCTION_CALL:
	{
		if (!token_len) {
			LOG(ERROR, "Missing request token");
			return ProtocolError::MISSING_REQUEST_TOKEN;
		}
		return functions.handle_function_call(token, msg_id, message, channel,
				descriptor.call_function);
	}

	case CoAPMessageType::VARIABLE_REQUEST:
	{
		if (!token_len) {
			LOG(ERROR, "Missing request token");
			return ProtocolError::MISSING_REQUEST_TOKEN;
		}
		return variables.handle_request(message, token, msg_id);
	}
#if HAL_PLATFORM_OTA_PROTOCOL_V3
	case CoAPMessageType::UPDATE_START_V3: {
		return firmwareUpdate.startRequest(&message);
	}
	case CoAPMessageType::UPDATE_FINISH_V3: {
		return firmwareUpdate.finishRequest(&message);
	}
	case CoAPMessageType::UPDATE_CHUNK_V3: {
		return firmwareUpdate.chunkRequest(&message);
	}
#else
	case CoAPMessageType::SAVE_BEGIN:
		// fall through
	case CoAPMessageType::UPDATE_BEGIN:
		return chunkedTransfer.handle_update_begin(token, message, channel);

	case CoAPMessageType::CHUNK:
		return chunkedTransfer.handle_chunk(token, message, channel);
	case CoAPMessageType::UPDATE_DONE:
		return chunkedTransfer.handle_update_done(token, message, channel);
#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3
	case CoAPMessageType::EVENT:
		return subscriptions.handle_event(message, descriptor.call_event_handler, channel);

	case CoAPMessageType::KEY_CHANGE:
		return handle_key_change(message);

	case CoAPMessageType::SIGNAL_START:
		message.set_length(
				Messages::coded_ack(message.buf(), token,
						ChunkReceivedCode::OK, queue[2], queue[3]));
		callbacks.signal(true, 0, NULL);
		return channel.send(message);

	case CoAPMessageType::SIGNAL_STOP:
		message.set_length(
				Messages::coded_ack(message.buf(), token,
						ChunkReceivedCode::OK, queue[2], queue[3]));
		callbacks.signal(false, 0, NULL);
		return channel.send(message);

	case CoAPMessageType::HELLO:
		if (message.get_type()==CoAPType::CON)
			send_empty_ack(message, msg_id);
		descriptor.ota_upgrade_status_sent();
		break;

	case CoAPMessageType::TIME:
		handle_time_response(
				queue[6] << 24 | queue[7] << 16 | queue[8] << 8 | queue[9]);
		break;

	case CoAPMessageType::PING:
		message.set_length(
				Messages::empty_ack(message.buf(), queue[2], queue[3]));
		error = channel.send(message);
		break;

	case CoAPMessageType::ERROR:
	default:
		; // drop it on the floor
	}

	// all's well
	return error;
}

void Protocol::notify_message_complete(message_id_t msg_id, CoAPCode::Enum responseCode) {
	const auto codeClass = (int)responseCode >> 5;
	const auto codeDetail = (int)responseCode & 0x1f;
	LOG(TRACE, "message id %d complete with code %d.%02d", msg_id, codeClass, codeDetail);
	if (CoAPCode::is_success(responseCode)) {
		ack_handlers.setResult(msg_id);
	} else {
		int error = SYSTEM_ERROR_COAP;
		switch (codeClass) {
		case 4:
			error = SYSTEM_ERROR_COAP_4XX;
			break;
		case 5:
			error = SYSTEM_ERROR_COAP_5XX;
			break;
		}
		ack_handlers.setError(msg_id, error);
	}
}

ProtocolError Protocol::handle_key_change(Message& message)
{
	uint8_t* buf = message.buf();
	ProtocolError result = NO_ERROR;
	if (CoAP::type(buf)==CoAPType::CON)
	{
		Message response;
		channel.response(message, response, 5);
		size_t sz = Messages::empty_ack(response.buf(), 0, 0);
		response.set_length(sz);
		result = channel.send(response);
	}

	// 4 bytes coAP header, 2 bytes message type option
	// token length, and skip 1 byte for the parameter option header.
	if (message.length()>7)
	{
		uint8_t* buf = message.buf();
		uint8_t option_idx = 7 + (buf[0] & 0xF);
		if (buf[option_idx]==1)
		{
			result = channel.command(MessageChannel::DISCARD_SESSION);
		}
	}
	return result;
}


/**
 * Handles the time delivered from the cloud.
 */
void Protocol::handle_time_response(uint32_t time)
{
	// deduct latency
	//uint32_t latency = last_chunk_millis ? (callbacks.millis()-last_chunk_millis)/2000 : 0;
	//last_chunk_millis = 0;
	// todo - compute connection latency
	timesync_.handle_time_response(time, callbacks.millis(), callbacks.set_time);
}

/**
 * Copy an initialize a block of memory from a source to a target, where the source may be smaller than the target.
 * This handles the case where the caller was compiled using a smaller version of the struct memory than what is the current.
 *
 * @param target			The destination structure
 * @param target_size 	The size of the destination structure in bytes
 * @param source			The source structure
 * @param source_size	The size of the source structure in bytes
 */
void Protocol::copy_and_init(void* target, size_t target_size,
		const void* source, size_t source_size)
{
	memcpy(target, source, source_size);
	memset(((uint8_t*) target) + source_size, 0, target_size - source_size);
}

void Protocol::init(const SparkCallbacks &callbacks,
		const SparkDescriptor &descriptor)
{
	memset(&handlers, 0, sizeof(handlers));
	// the actual instances referenced may be smaller if the caller is compiled
	// against an older version of this library.
	copy_and_init(&this->callbacks, sizeof(this->callbacks), &callbacks,
			callbacks.size);
	copy_and_init(&this->descriptor, sizeof(this->descriptor), &descriptor,
			descriptor.size);

#if HAL_PLATFORM_OTA_PROTOCOL_V3
	firmwareUpdate.init(&channel, this->callbacks);
#else
	chunkedTransferCallbacks.init(&this->callbacks);
	chunkedTransfer.init(&chunkedTransferCallbacks);
#endif

	initialized = true;
}

AppStateDescriptor Protocol::app_state_descriptor(uint32_t stateFlags)
{
	if (!descriptor.app_state_selector_info) {
		return AppStateDescriptor();
	}
	AppStateDescriptor d;
	if (stateFlags & AppStateDescriptor::SYSTEM_DESCRIBE_CRC) {
		d.systemDescribeCrc(descriptor.app_state_selector_info(SparkAppStateSelector::DESCRIBE_SYSTEM, SparkAppStateUpdate::COMPUTE, 0, nullptr));
	}
	if (stateFlags & AppStateDescriptor::APP_DESCRIBE_CRC) {
		d.appDescribeCrc(descriptor.app_state_selector_info(SparkAppStateSelector::DESCRIBE_APP, SparkAppStateUpdate::COMPUTE, 0, nullptr));
	}
	if (stateFlags & AppStateDescriptor::SUBSCRIPTIONS_CRC) {
		d.subscriptionsCrc(subscriptions.compute_subscriptions_checksum(callbacks.calculate_crc));
	}
	if (stateFlags & AppStateDescriptor::PROTOCOL_FLAGS) {
		d.protocolFlags(protocol_flags);
	}
	return d;
}

/**
 * Establish a secure connection and send and process the hello message.
 */
int Protocol::begin()
{
	LOG_CATEGORY("comm.protocol.handshake");
	LOG(INFO,"Establish secure connection");

	reset();
	last_ack_handlers_update = callbacks.millis();

	ProtocolError error = channel.establish();
	const bool session_resumed = (error == SESSION_RESUMED);
	if (error && !session_resumed) {
		LOG(ERROR, "Handshake failed: %d", error);
		return error;
	}

	if (session_resumed) {
		// for now, unconditionally move the session on resumption
		channel.command(MessageChannel::MOVE_SESSION, nullptr);
		uint32_t stateFlags = AppStateDescriptor::ALL;
		if (protocol_flags & ProtocolFlag::DEVICE_INITIATED_DESCRIBE) {
			// The system controls when to send application Describe and subscriptions
			stateFlags = AppStateDescriptor::SYSTEM_DESCRIBE_CRC | AppStateDescriptor::PROTOCOL_FLAGS;
		}
		const auto currentState = app_state_descriptor(stateFlags);
		const auto cachedState = channel.cached_app_state_descriptor();
		if (currentState.equalsTo(cachedState, stateFlags)) {
			LOG(INFO, "Skipping HELLO message");
			error = ping(true);
			if (error != ProtocolError::NO_ERROR) {
				return error;
			}
			return ProtocolError::SESSION_RESUMED; // Not an error
		} else {
			// TODO: For now, make sure application Describe and subscriptions will be sent if the
			// system state has changed
			channel.command(Channel::SAVE_SESSION);
			descriptor.app_state_selector_info(SparkAppStateSelector::ALL, SparkAppStateUpdate::RESET, 0, nullptr);
			channel.command(Channel::LOAD_SESSION);
		}
	}

	LOG(INFO, "Sending HELLO message");
	error = hello(descriptor.was_ota_upgrade_successful());
	if (error) {
		LOG(ERROR,"Could not send HELLO message: %d", error);
		return error;
	}

	if (protocol_flags & ProtocolFlag::REQUIRE_HELLO_RESPONSE) {
		LOG(INFO, "Receiving HELLO response");
		error = hello_response();
		if (error) {
			return error;
		}
	}

	LOG(INFO, "Handshake completed");
	channel.notify_established();

	// An ACK or a response for the Hello message has already been received at this point, so we can
	// update cached protocol flags
	if (descriptor.app_state_selector_info) {
		LOG(TRACE, "Updating protocol flags");
		channel.command(Channel::SAVE_SESSION);
		descriptor.app_state_selector_info(SparkAppStateSelector::PROTOCOL_FLAGS, SparkAppStateUpdate::PERSIST, protocol_flags, nullptr);
		channel.command(Channel::LOAD_SESSION);
	}

	if (protocol_flags & ProtocolFlag::DEVICE_INITIATED_DESCRIBE) {
		// Send system Describe
		error = post_description(DescriptionType::DESCRIBE_SYSTEM, true /* force */);
		if (error) {
			return error;
		}
	}

	// TODO: This will cause all the application events to be sent even if the session was resumed
	return 0;
}

void Protocol::reset() {
#if HAL_PLATFORM_OTA_PROTOCOL_V3
	firmwareUpdate.reset();
#else
	chunkedTransfer.reset();
#endif
	pinger.reset();
	timesync_.reset();
	ack_handlers.clear();
	channel.reset();
	app_describe_msg_id = INVALID_MESSAGE_HANDLE;
	system_describe_msg_id = INVALID_MESSAGE_HANDLE;
	subscription_msg_ids.clear();
}

/**
 * Send the hello message over the channel.
 * @param was_ota_upgrade_successful {@code true} if the previous OTA update was successful.
 */
ProtocolError Protocol::hello(bool was_ota_upgrade_successful)
{
	Message message;
	channel.create(message);

	uint8_t flags = HELLO_FLAG_DIAGNOSTICS_SUPPORT | HELLO_FLAG_IMMEDIATE_UPDATES_SUPPORT |
			HELLO_FLAG_GOODBYE_SUPPORT;
	if (was_ota_upgrade_successful) {
		flags |= HELLO_FLAG_OTA_UPGRADE_SUCCESSFUL;
	}
	if (protocol_flags & ProtocolFlag::DEVICE_INITIATED_DESCRIBE) {
		flags |= HELLO_FLAG_DEVICE_INITIATED_DESCRIBE;
	}
	if (protocol_flags & ProtocolFlag::COMPRESSED_OTA) {
		flags |= HELLO_FLAG_COMPRESSED_OTA;
	}
#if HAL_PLATFORM_OTA_PROTOCOL_V3
	flags |= HELLO_FLAG_OTA_PROTOCOL_V3;
#endif
	size_t len = build_hello(message, flags);
	message.set_length(len);
	message.set_confirm_received(true); // Send synchronously
	last_message_millis = callbacks.millis();
	return channel.send(message);
}

ProtocolError Protocol::hello_response()
{
	ProtocolError error = event_loop(CoAPMessageType::HELLO, 4000); // read the hello message from the server
	if (error)
	{
		LOG(ERROR, "Handshake: could not receive HELLO response %d", error);
	}
	return error;
}

/**
 * Wait for a specific message type to be received.
 * @param message_type		The type of message wait for
 * @param timeout			The duration to wait for the message before giving up.
 *
 * @returns NO_ERROR if the message was successfully matched within the timeout.
 * Returns MESSAGE_TIMEOUT if the message wasn't received within the timeout.
 * Other protocol errors may return additional error values.
 */
ProtocolError Protocol::event_loop(CoAPMessageType::Enum message_type,
		system_tick_t timeout)
{
	system_tick_t start = callbacks.millis();
	LOG(INFO,"waiting %d seconds for message type=%d", timeout/1000, message_type);
	do
	{
		CoAPMessageType::Enum msgtype;
		ProtocolError error = event_loop(msgtype);
		if (error) {
			LOG(ERROR,"message type=%d, error=%d", (int)msgtype, error);
			return error;
		}
		if (msgtype == message_type)
			return NO_ERROR;
		// todo - ideally need a delay here
	}
	while ((callbacks.millis() - start) < timeout);
	return MESSAGE_TIMEOUT;
}

/**
 * Processes one event. Retrieves the type of the event processed, or NONE if no event processed.
 * If an error occurs, the event type is undefined.
 */
ProtocolError Protocol::event_loop(CoAPMessageType::Enum& message_type)
{
	// Process expired completion handlers
	const system_tick_t t = callbacks.millis();
	ack_handlers.update(t - last_ack_handlers_update);
	last_ack_handlers_update = t;

	Message message;
	message_type = CoAPMessageType::NONE;
	ProtocolError error = channel.receive(message);
	if (!error)
	{
		if (message.length())
		{
			error = handle_received_message(message, message_type);
			LOG(TRACE, "rcv'd message type=%d", (int)message_type);
		}
		else
		{
			error = event_loop_idle();
		}
	}

	if (error)
	{
		// bail if and only if there was an error
#if HAL_PLATFORM_OTA_PROTOCOL_V3
		firmwareUpdate.reset();
#else
		chunkedTransfer.cancel();
#endif
		LOG(ERROR,"Event loop error %d", error);
		return error;
	}
	return error;
}

void Protocol::build_describe_message(Appender& appender, int desc_flags)
{
	// diagnostics must be requested in isolation to be a binary packet
	if (descriptor.append_metrics && (desc_flags == DESCRIBE_METRICS))
	{
		appender.append(char(0));	// null byte means binary data
		appender.append(char(DESCRIBE_METRICS));	// uint16 describes the type of binary packet
		appender.append(char(0));	//
		const int flags = 1;		// binary
		const int page = 0;
		descriptor.append_metrics(Appender::callback, &appender, flags, page, nullptr);
	}
	else {
		appender.append("{");
		bool has_content = false;

		if (desc_flags & DESCRIBE_APPLICATION)
		{
			has_content = true;
			appender.append("\"f\":[");

			int num_keys = descriptor.num_functions();
			int i;
			for (i = 0; i < num_keys; ++i)
			{
				if (i)
				{
					appender.append(',');
				}
				appender.append('"');

				const char* key = descriptor.get_function_key(i);
				size_t function_name_length = strlen(key);
				if (MAX_FUNCTION_KEY_LENGTH < function_name_length)
				{
					function_name_length = MAX_FUNCTION_KEY_LENGTH;
				}
				appender.append((const uint8_t*) key, function_name_length);
				appender.append('"');
			}

			appender.append("],\"v\":{");

			num_keys = descriptor.num_variables();
			for (i = 0; i < num_keys; ++i)
			{
				if (i)
				{
					appender.append(',');
				}
				appender.append('"');
				const char* key = descriptor.get_variable_key(i);
				size_t variable_name_length = strlen(key);
				SparkReturnType::Enum t = descriptor.variable_type(key);
				if (MAX_VARIABLE_KEY_LENGTH < variable_name_length)
				{
					variable_name_length = MAX_VARIABLE_KEY_LENGTH;
				}
				appender.append((const uint8_t*) key, variable_name_length);
				appender.append("\":");
				appender.append('0' + (char) t);
			}
			appender.append('}');
		}

		if (descriptor.append_system_info && (desc_flags & DESCRIBE_SYSTEM))
		{
			if (has_content)
			{
				appender.append(',');
			}
			has_content = true;
			descriptor.append_system_info(Appender::callback, &appender, nullptr);
		}
		appender.append('}');
	}
}

ProtocolError Protocol::generate_and_send_description(MessageChannel& channel, Message& message,
                                                      size_t header_size, int desc_flags)
{
    ProtocolError error;

    BufferAppender appender((message.buf() + header_size), (message.capacity() - header_size));
    build_describe_message(appender, desc_flags);

    if (appender.dataSize() > appender.bufferSize())
    {
        LOG(ERROR, "Describe message overflowed by %u bytes", (unsigned)(appender.dataSize() - appender.bufferSize()));
        // There is no point in continuing to run, the device will be constantly reconnecting
        // to the cloud. It's better to clearly indicate that the describe message is never going
        // to go through to the cloud by going into a panic state, otherwise one would have to
        // sift through logs to find 'Describe message overflowed by %d bytes' message to understand
        // what's going on.
        SPARK_ASSERT(false);
    }

    message.set_length(header_size + appender.dataSize());

    LOG(INFO, "Posting '%s%s%s' describe message", desc_flags & DESCRIBE_SYSTEM ? "S" : "",
        desc_flags & DESCRIBE_APPLICATION ? "A" : "", desc_flags & DESCRIBE_METRICS ? "M" : "");

    error = channel.send(message);
    if (error != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Channel failed to send message; error code: %d", (int)error);
    } else if (descriptor.app_state_selector_info) {
        const auto msg_id = message.get_id();
        if (desc_flags & DescriptionType::DESCRIBE_APPLICATION) {
            app_describe_msg_id = msg_id;
        }
        if (desc_flags & DescriptionType::DESCRIBE_SYSTEM) {
            system_describe_msg_id = msg_id;
        }
    }

    return error;
}

ProtocolError Protocol::post_description(int desc_flags, bool force)
{
	if (!force && descriptor.app_state_selector_info) {
		const auto cachedState = channel.cached_app_state_descriptor();
		if (desc_flags & DescriptionType::DESCRIBE_SYSTEM) {
			const auto currentState = app_state_descriptor(AppStateDescriptor::SYSTEM_DESCRIBE_CRC);
			if (currentState.equalsTo(cachedState, AppStateDescriptor::SYSTEM_DESCRIBE_CRC)) {
				LOG(INFO, "Checksum has not changed; not sending system DESCRIBE");
				desc_flags &= ~DescriptionType::DESCRIBE_SYSTEM;
			}
		}
		if (desc_flags & DescriptionType::DESCRIBE_APPLICATION) {
			const auto currentState = app_state_descriptor(AppStateDescriptor::APP_DESCRIBE_CRC);
			if (currentState.equalsTo(cachedState, AppStateDescriptor::APP_DESCRIBE_CRC)) {
				LOG(INFO, "Checksum has not changed; not sending application DESCRIBE");
				desc_flags &= ~DescriptionType::DESCRIBE_APPLICATION;
			}
		}
	}
	if (!desc_flags) {
		return ProtocolError::NO_ERROR;
	}
	Message message;
	const ProtocolError error = channel.create(message);
	if (error != ProtocolError::NO_ERROR) {
		return error;
	}
	const size_t header_size = Messages::describe_post_header(message.buf(), message.capacity(), 0 /* message_id */, desc_flags);
	return generate_and_send_description(channel, message, header_size, desc_flags);
}

ProtocolError Protocol::send_description_response(token_t token, message_id_t msg_id, int desc_flags)
{
	// Acknowledge the request
	Message msg;
	ProtocolError error = channel.create(msg);
	if (error != ProtocolError::NO_ERROR) {
		return error;
	}
	error = send_empty_ack(msg, msg_id);
	if (error != ProtocolError::NO_ERROR) {
		return error;
	}
	// Send a response
	error = channel.create(msg);
	if (error != ProtocolError::NO_ERROR) {
		return error;
	}
	const auto buf = msg.buf();
	const size_t size = Messages::description_response(buf, 0 /* message_id */, token);
	return generate_and_send_description(channel, msg, size, desc_flags);
}

ProtocolError Protocol::send_subscription(const char *event_name, const char *device_id)
{
	const ProtocolError error = subscriptions.send_subscription(channel, event_name, device_id);
	if (error == ProtocolError::NO_ERROR && descriptor.app_state_selector_info) {
		subscription_msg_ids.append(subscriptions.subscription_message_ids());
	}
	return error;
}

ProtocolError Protocol::send_subscription(const char *event_name, SubscriptionScope::Enum scope)
{
	const ProtocolError error = subscriptions.send_subscription(channel, event_name, scope);
	if (error == ProtocolError::NO_ERROR && descriptor.app_state_selector_info) {
		subscription_msg_ids.append(subscriptions.subscription_message_ids());
	}
	return error;
}

ProtocolError Protocol::send_subscriptions(bool force)
{
	if (!force && descriptor.app_state_selector_info) {
		const auto currentState = app_state_descriptor(AppStateDescriptor::SUBSCRIPTIONS_CRC);
		const auto cachedState = channel.cached_app_state_descriptor();
		if (currentState.equalsTo(cachedState, AppStateDescriptor::SUBSCRIPTIONS_CRC)) {
			LOG(INFO, "Checksum has not changed; not sending subscriptions");
			return ProtocolError::NO_ERROR;
		}
	}
	LOG(INFO, "Sending subscriptions");
	const ProtocolError error = subscriptions.send_subscriptions(channel);
	if (error == ProtocolError::NO_ERROR && descriptor.app_state_selector_info) {
		subscription_msg_ids.append(subscriptions.subscription_message_ids());
	}
	return error;
}

bool Protocol::handle_app_state_reply(message_id_t msg_id, CoAPCode::Enum code)
{
	if (!descriptor.app_state_selector_info) {
		return false;
	}
	// Update application state checksums
	if (subscription_msg_ids.removeOne(msg_id)) { // Event subscriptions
		if (!CoAPCode::is_success(code)) {
			// Make sure the checksum won't be updated if any of the messages has been NAK'ed
			subscription_msg_ids.clear();
		} else if (subscription_msg_ids.isEmpty()) {
			LOG(TRACE, "Updating subscriptions checksum");
			const uint32_t crc = subscriptions.compute_subscriptions_checksum(callbacks.calculate_crc);
			channel.command(Channel::SAVE_SESSION);
			descriptor.app_state_selector_info(SparkAppStateSelector::SUBSCRIPTIONS,
					SparkAppStateUpdate::PERSIST, crc, nullptr);
			channel.command(Channel::LOAD_SESSION);
		}
		return true;
	}
	// A Describe message can carry both the system and application descriptions
	if (msg_id == app_describe_msg_id || msg_id == system_describe_msg_id) {
		if (msg_id == app_describe_msg_id) { // Application description
			app_describe_msg_id = INVALID_MESSAGE_HANDLE;
			if (CoAPCode::is_success(code)) {
				LOG(TRACE, "Updating application DESCRIBE checksum");
				channel.command(Channel::SAVE_SESSION);
				descriptor.app_state_selector_info(SparkAppStateSelector::DESCRIBE_APP,
						SparkAppStateUpdate::COMPUTE_AND_PERSIST, 0, nullptr);
				channel.command(Channel::LOAD_SESSION);
			}
		}
		if (msg_id == system_describe_msg_id) { // System description
			system_describe_msg_id = INVALID_MESSAGE_HANDLE;
			if (CoAPCode::is_success(code)) {
				LOG(TRACE, "Updating system DESCRIBE checksum");
				channel.command(Channel::SAVE_SESSION);
				descriptor.app_state_selector_info(SparkAppStateSelector::DESCRIBE_SYSTEM,
						SparkAppStateUpdate::COMPUTE_AND_PERSIST, 0, nullptr);
				channel.command(Channel::LOAD_SESSION);
			}
		}
		return true;
	}
	return false;
}

#if !HAL_PLATFORM_OTA_PROTOCOL_V3

int Protocol::ChunkedTransferCallbacks::prepare_for_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void* reserved)
{
	return callbacks->prepare_for_firmware_update(data, flags, reserved);
}

int Protocol::ChunkedTransferCallbacks::save_firmware_chunk(FileTransfer::Descriptor& descriptor, const unsigned char* chunk, void* reserved)
{
	return callbacks->save_firmware_chunk(descriptor, chunk, reserved);
}

int Protocol::ChunkedTransferCallbacks::finish_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void* reserved)
{
	return callbacks->finish_firmware_update(data, flags, reserved);
}

uint32_t Protocol::ChunkedTransferCallbacks::calculate_crc(const unsigned char *buf, uint32_t buflen)
{
	return callbacks->calculate_crc(buf, buflen);
}

system_tick_t Protocol::ChunkedTransferCallbacks::millis()
{
	return callbacks->millis();
}

#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3

int Protocol::get_describe_data(spark_protocol_describe_data* data, void* reserved)
{
	data->maximum_size = 768;  // a conservative guess based on dtls and lightssl encryption overhead and the CoAP data
	BufferAppender appender(nullptr,  0);	// don't need to store the data, just count the size
	build_describe_message(appender, data->flags);
	data->current_size = appender.dataSize();
	return 0;
}

}}
