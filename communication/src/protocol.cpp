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

#include "protocol.h"
#include "chunked_transfer.h"
#include "subscriptions.h"
#include "functions.h"

namespace particle { namespace protocol {


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
	token_t token = queue[4];
	message_id_t msg_id = CoAP::message_id(queue);
	ProtocolError error = NO_ERROR;
	//DEBUG("message type %d", message_type);
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
		return functions.handle_function_call(token, msg_id, message, channel,
				descriptor.call_function);

	case CoAPMessageType::VARIABLE_REQUEST:
	{
		char variable_key[13];
		variables.decode_variable_request(variable_key, message);
		return variables.handle_variable_request(variable_key, message,
				channel, token, msg_id,
				descriptor.variable_type, descriptor.get_variable);
	}
	case CoAPMessageType::SAVE_BEGIN:
		// fall through
	case CoAPMessageType::UPDATE_BEGIN:
		return chunkedTransfer.handle_update_begin(token, message, channel);

	case CoAPMessageType::CHUNK:
		return chunkedTransfer.handle_chunk(token, message, channel);
	case CoAPMessageType::UPDATE_DONE:
		return chunkedTransfer.handle_update_done(token, message, channel);

	case CoAPMessageType::EVENT:
		return subscriptions.handle_event(message, descriptor.call_event_handler);

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
void Protocol::handle_time_response(uint32_t time)
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

	chunkedTransferCallbacks.init(&this->callbacks);
	chunkedTransfer.init(&chunkedTransferCallbacks);

	initialized = true;
}


/**
 * Establish a secure connection and send and process the hello message.
 */
int Protocol::begin()
{
	chunkedTransfer.reset();
	pinger.reset();

	ProtocolError error = channel.establish();
	if (error) {
		WARN("handshake failed with code %d", error);
		return error;
	}
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
ProtocolError Protocol::event_loop(CoAPMessageType::Enum message_type,
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
ProtocolError Protocol::event_loop(CoAPMessageType::Enum& message_type)
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
		chunkedTransfer.cancel();
		WARN("Event loop error %d", error);
		return error;
	}
	return error;
}


/**
 * Produces and transmits a describe message.
 * @param desc_flags Flags describing the information to provide. A combination of {@code DESCRIBE_APPLICATION) and {@code DESCRIBE_SYSTEM) flags.
 */
ProtocolError Protocol::send_description(token_t token, message_id_t msg_id, int desc_flags)
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


}}
