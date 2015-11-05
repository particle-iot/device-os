#pragma once

#include "message_channel.h"
#include "service_debug.h"
#include "protocol_defs.h"
#include "ping.h"
#include "chunked_transfer.h"
#include "spark_descriptor.h"
#include "spark_protocol_functions.h"
#include "functions.h"
#include "events.h"
#include "publisher.h"
#include "subscriptions.h"
#include "variables.h"
#include "hal_platform.h"

namespace particle
{
namespace protocol
{

/**
 * Tie ALL the bits together.
 */
class Protocol
{
	/**
	 * The message channel that sends and receives message packets.
	 */
	MessageChannel& channel;

	/**
	 * The tick time of the last communication with the cloud.
	 * todo - move this into the message channel?
	 */
	system_tick_t last_message_millis;

	/**
	 * The product_id represented by this device. set_product_id()
	 */
	product_id_t product_id;

	/**
	 * The product version for this device.
	 */
	product_firmware_version_t product_firmware_version;

	/**
	 * Descriptor callbacks that provide externally hosted functions and variables.
	 */
	SparkDescriptor descriptor;

	/**
	 * Functional callbacks that provide key system services to this communications layer.
	 */
	SparkCallbacks callbacks;

	/**
	 * Application-level callbacks.
	 */
	CommunicationsHandlers handlers;

	/**
	 * Manages Ping functionality.
	 */
	Pinger pinger;

	/**
	 * Manages chunked file transfer functionality.
	 */
	ChunkedTransfer chunkedTransfer;

	/**
	 * Manages device-hosted variables.
	 */
	Variables variables;

	/**
	 * Manages device-hosted functions.
	 */
	Functions functions;

	/**
	 * Manages subscriptions from this device.
	 */
	Subscriptions subscriptions;

	/**
	 * Manages published events from this device.
	 */
	Publisher publisher;

	/**
	 * The token ID for the next request made.
	 * If we have a bone-fide CoAP layer this will eventually disappear into that layer, just like message-id has.
	 */
	token_t token;

	uint8_t initialized;

protected:

	/**
	 * Retrieves the next token.
	 */
	uint8_t next_token()
	{
		return ++token;
	}

	/**
	 * Send the hello message over the channel.
	 * @param was_ota_upgrade_successful {@code true} if the previous OTA update was successful.
	 */
	ProtocolError hello(bool was_ota_upgrade_successful)
	{
		Message message;
		channel.create(message);
		size_t len = Messages::hello(message.buf(), 0,
				was_ota_upgrade_successful, PLATFORM_ID, product_id,
				product_firmware_version);
		message.set_length(len);
		last_message_millis = callbacks.millis();
		return channel.send(message);
	}

	/**
	 * Send a Ping message over the channel.
	 */
	ProtocolError ping()
	{
		Message message;
		channel.create(message);
		size_t len = Messages::ping(message.buf(), 0);
		last_message_millis = callbacks.millis();
		message.set_length(len);
		return channel.send(message);
	}

	/**
	 * Background processing when there are no messages to handle.
	 */
	ProtocolError event_loop_idle()
	{
		if (chunkedTransfer.is_updating())
		{
			return chunkedTransfer.idle(channel, callbacks.millis);
		}
		else
		{
			ProtocolError error = pinger.process(
					callbacks.millis() - last_message_millis, [this]
					{	return ping();});
			if (error)
				return error;
		}
		return NO_ERROR;
	}

	/**
	 * The number of missed chunks to send in a single flight.
	 */
	const int MISSED_CHUNKS_TO_SEND = 50;

	/**
	 * Produces and transmits a describe message.
	 * @param desc_flags Flags describing the information to provide. A combination of {@code DESCRIBE_APPLICATION) and {@code DESCRIBE_SYSTEM) flags.
	 */
	ProtocolError send_description(token_t token, message_id_t msg_id,
			int desc_flags)
	{
		Message message;
		channel.create(message);
		uint8_t* buf = message.buf();
		message.set_id(msg_id);
		size_t desc = Messages::description(buf, msg_id, token);

		BufferAppender appender(buf + desc, message.capacity());
		appender.append("{");
		bool has_content = false;

		if (desc_flags && DESCRIBE_APPLICATION)
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
				appender.append(',');
			descriptor.append_system_info(append_instance, &appender, nullptr);
		}
		appender.append('}');
		int msglen = appender.next() - (uint8_t *) buf;
		message.set_length(msglen);
		return channel.send(message);
	}

	/**
	 * Decodes and dispatches a received message to its handler.
	 */
	ProtocolError handle_received_message(Message& message,
			CoAPMessageType::Enum& message_type)
	{
		last_message_millis = callbacks.millis();
		pinger.message_received();
		uint8_t* queue = message.buf();
		message_type = Messages::decodeType(queue, message.length());
		token_t token = queue[4];
		message_id_t msg_id = CoAP::message_id(queue);
		ProtocolError error = NO_ERROR;
		DEBUG("message type %d", message_type);
		switch (message_type)
		{
		case CoAPMessageType::DESCRIBE:
			// TODO - this violates CoAP since we are sending two different achnowledgements
			error = send_description(token, msg_id,
					DESCRIBE_SYSTEM | DESCRIBE_APPLICATION);
			//if (!error)
			//		error = send_description(token, msg_id, DESCRIBE_APPLICATION);
			break;

		case CoAPMessageType::FUNCTION_CALL:
			return functions.handle_function_call(token, message, channel,
					descriptor.call_function);

		case CoAPMessageType::VARIABLE_REQUEST:
		{
			char variable_key[13];
			variables.decode_variable_request(variable_key, message);
			return variables.handle_variable_request(variable_key, message,
					channel, token, CoAP::message_id(queue),
					descriptor.variable_type, descriptor.get_variable);
		}
		case CoAPMessageType::SAVE_BEGIN:
			// fall through
		case CoAPMessageType::UPDATE_BEGIN:
			return chunkedTransfer.handle_update_begin(token, message, channel,
					callbacks.prepare_for_firmware_update, callbacks.millis);

		case CoAPMessageType::CHUNK:
			return chunkedTransfer.handle_chunk(token, message, channel,
					callbacks.save_firmware_chunk, callbacks.calculate_crc,
					callbacks.finish_firmware_update, callbacks.millis);

		case CoAPMessageType::UPDATE_DONE:
			return chunkedTransfer.handle_update_done(token, message, channel,
					callbacks.finish_firmware_update, callbacks.millis);

		case CoAPMessageType::EVENT:
			return subscriptions.handle_event(message,
					descriptor.call_event_handler);

		case CoAPMessageType::KEY_CHANGE:
			// TODO
			break;

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

		case CoAPMessageType::EMPTY_ACK:
		case CoAPMessageType::ERROR:
		default:
			; // drop it on the floor
		}

		// all's well
		return error;
	}

	/**
	 * Handles the time delivered from the cloud.
	 */
	void handle_time_response(uint32_t time)
	{
		// deduct latency
		//uint32_t latency = last_chunk_millis ? (callbacks.millis()-last_chunk_millis)/2000 : 0;
		//last_chunk_millis = 0;
		// todo - compute connection latency
		callbacks.set_time(time, 0, NULL);
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
	static void copy_and_init(void* target, size_t target_size,
			const void* source, size_t source_size)
	{
		memcpy(target, source, source_size);
		memset(((uint8_t*) target) + source_size, 0, target_size - source_size);
	}

	void init(const SparkCallbacks &callbacks,
			const SparkDescriptor &descriptor)
	{
		memset(&handlers, 0, sizeof(handlers));
		// the actual instances referenced may be smaller if the caller is compiled
		// against an older version of this library.
		copy_and_init(&this->callbacks, sizeof(this->callbacks), &callbacks,
				callbacks.size);
		copy_and_init(&this->descriptor, sizeof(this->descriptor), &descriptor,
				descriptor.size);

		initialized = true;
	}

public:
	Protocol(MessageChannel& channel) :
			channel(channel), initialized(false)
	{
	}

	virtual void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor)=0;

	void set_handlers(CommunicationsHandlers& handlers)
	{
		copy_and_init(&this->handlers, sizeof(this->handlers), &handlers,
				handlers.size);
	}

	/**
	 * Establish a secure connection and send and process the hello message.
	 */
	int begin()
	{
		chunkedTransfer.reset();
		pinger.reset();

		ProtocolError error = channel.establish();
		if (error)
			return error;

		error = hello(descriptor.was_ota_upgrade_successful());
		if (error)
		{
			ERROR("Hanshake: could not send hello message: %d", error);
			return error;
		}

		error = event_loop(CoAPMessageType::HELLO, 2000); // read the hello message from the server
		if (error)
		{
			ERROR("Handshake: could not receive hello response %d", error);
			return error;
		}

		INFO("Hanshake: completed");
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
	ProtocolError event_loop(CoAPMessageType::Enum message_type,
			system_tick_t timeout)
	{
		system_tick_t start = callbacks.millis();
		do
		{
			CoAPMessageType::Enum msgtype;
			ProtocolError error = event_loop(msgtype);
			if (error)
				return error;
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
	ProtocolError event_loop(CoAPMessageType::Enum& message_type)
	{
		Message message;
		message_type = CoAPMessageType::NONE;
		ProtocolError error = channel.receive(message);
		if (!error)
		{
			if (message.length())
			{
				error = handle_received_message(message, message_type);
			}
			else
			{
				error = event_loop_idle();
			}
		}

		if (error)
		{
			// bail if and only if there was an error
			chunkedTransfer.cancel(callbacks.finish_firmware_update);
			WARN("Event loop error %d", error);
			return error;
		}
		return error;
	}

	/**
	 * no-arg version of event loop for those callers that don't care about the message.
	 */
	bool event_loop()
	{
		CoAPMessageType::Enum message;
		return !event_loop(message);
	}

	// Returns true on success, false on sending timeout or rate-limiting failure
	bool send_event(const char *event_name, const char *data, int ttl,
			EventType::Enum event_type)
	{
		if (chunkedTransfer.is_updating())
		{
			return false;
		}
		return !publisher.send_event(channel, event_name, data, ttl, event_type,
				callbacks.millis());
	}

	inline bool send_subscription(const char *event_name, const char *device_id)
	{
		return !subscriptions.send_subscription(channel, event_name, device_id);
	}

	inline bool send_subscription(const char *event_name,
			SubscriptionScope::Enum scope)
	{
		return !subscriptions.send_subscription(channel, event_name, scope);
	}

	inline bool add_event_handler(const char *event_name, EventHandler handler)
	{
		return add_event_handler(event_name, handler, NULL,
				SubscriptionScope::FIREHOSE, NULL);
	}

	inline bool add_event_handler(const char *event_name, EventHandler handler,
			void *handler_data, SubscriptionScope::Enum scope,
			const char* device_id)
	{
		return !subscriptions.add_event_handler(event_name, handler,
				handler_data, scope, device_id);
	}

	inline bool send_subscriptions()
	{
		return !subscriptions.send_subscriptions(channel);
	}

	inline bool remove_event_handlers(const char* name)
	{
		subscriptions.remove_event_handlers(name);
		return true;
	}

	inline void set_product_id(product_id_t product_id)
	{
		this->product_id = product_id;
	}

	inline void set_product_firmware_version(
			product_firmware_version_t product_firmware_version)
	{
		this->product_firmware_version = product_firmware_version;
	}

	inline void get_product_details(product_details_t& details)
	{
		if (details.size >= 4)
		{
			details.product_id = this->product_id;
			details.product_version = this->product_firmware_version;
		}
	}

	inline bool send_time_request()
	{
		if (chunkedTransfer.is_updating())
		{
			return false;
		}

		uint8_t token = next_token();
		Message message;
		channel.create(message);
		size_t len = Messages::time_request(message.buf(), 0, token);
		message.set_length(len);
		return !channel.send(message);
	}

	bool is_initialized() { return initialized; }

	int presence_announcement(uint8_t* buf, const uint8_t* id)
	{
		return -1;
	}

};

}
}
