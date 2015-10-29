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
#include "subscriptions.h"
#include "variables.h"

namespace particle
{
namespace protocol
{

class Protocol
{
	MessageChannel& channel;

	system_tick_t last_message_millis;
	message_id_t _message_id;
    product_id_t product_id;
    product_firmware_version_t product_firmware_version;

    Pinger pinger;
    ChunkedTransfer chunkedTransfer;
    Functions functions;
    Subscriptions subscriptions;
    SparkDescriptor descriptor;
    SparkCallbacks callbacks;
    Variables variables;

	message_id_t next_message_id()
	{
	  return ++_message_id;
	}

protected:


	ProtocolError hello(bool was_ota_upgrade_successful)
	{
		Message message;
		channel.create(message);
		size_t len = Messages::hello(message.buf(), next_message_id(), was_ota_upgrade_successful, PLATFORM_ID, product_id, product_firmware_version);
		message.set_length(len);
		return channel.send(message);
	}

	ProtocolError ping()
	{
		Message message;
		channel.create(message);
		size_t len = Messages::ping(message.buf(), next_message_id());
		message.set_length(len);
		return channel.send(message);
	}


	ProtocolError event_loop_idle()
	{
		if (chunkedTransfer.is_updating())
		{
			return chunkedTransfer.idle(channel, callbacks.millis, [this]{ return next_message_id(); });
		}
		else
		{
			ProtocolError error = pinger.process(last_message_millis-callbacks.millis(), [this]{return ping();});
			if (error) return error;
		}
		return NO_ERROR;
	}

	void sent_subscription(const FilteringEventHandler& handler)
	{
	    uint16_t msg_id = next_message_id();
	    size_t msglen;
	    Message message;
	    channel.create(message);
        if (handler.device_id[0])
       	  msglen = subscription(message.buf(), msg_id, handler.filter, handler.device_id);
        else
           msglen = subscription(message.buf(), msg_id, handler.filter, handler.scope);
        message.set_length(msglen);
        channel.send(message);
	}

	const int MISSED_CHUNKS_TO_SEND = 50;

	ProtocolError send_description(unsigned char token, int desc_flags)
	{
		uint16_t message_id = next_message_id();
		Message message;
		channel.create(message);
		uint8_t* buf = message.buf();
		size_t desc = Messages::description(buf, message_id, token);

		BufferAppender appender(buf+desc, message.capacity()-8);
	    appender.append("{");
	    bool has_content = false;

	    if (desc_flags && DESCRIBE_APPLICATION) {
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
	        int function_name_length = strlen(key);
	        if (MAX_FUNCTION_KEY_LENGTH < function_name_length)
	        {
	          function_name_length = MAX_FUNCTION_KEY_LENGTH;
	        }
	        appender.append((const uint8_t*)key, function_name_length);
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
	        int variable_name_length = strlen(key);
	        SparkReturnType::Enum t = descriptor.variable_type(key);
	        if (MAX_VARIABLE_KEY_LENGTH < variable_name_length)
	        {
	          variable_name_length = MAX_VARIABLE_KEY_LENGTH;
	        }
	        appender.append((const uint8_t*)key, variable_name_length);
	        appender.append("\":");
	        appender.append('0' + (char)t);
	      }
	      appender.append('}');
	    }

	    if (descriptor.append_system_info && (desc_flags&DESCRIBE_SYSTEM)) {
	      if (has_content)
	        appender.append(',');
	      descriptor.append_system_info(append_instance, &appender, NULL);
	    }
	    appender.append('}');
	    int msglen = appender.next() - (uint8_t *)buf;
	    message.set_length(msglen);
	    return channel.send(message);
	}

	ProtocolError handle_received_message(Message& message)
	{
	  last_message_millis = callbacks.millis();
	  pinger.message_received();
	  uint8_t* queue = message.buf();
	  CoAPMessageType::Enum message_type = Messages::decodeType(queue, message.length());

	  token_t token = queue[4];

	  ProtocolError error = NO_ERROR;
	  switch (message_type)
	  {
	    case CoAPMessageType::DESCRIBE:
	        error = send_description(token, DESCRIBE_SYSTEM);
	        if (!error)
	        		error = send_description(token, DESCRIBE_APPLICATION);
	        break;

	    case CoAPMessageType::FUNCTION_CALL:
	        return functions.handle_function_call([this]{return next_message_id();}, token, message, channel, descriptor.call_function);

	    case CoAPMessageType::VARIABLE_REQUEST:
	    {
	    		char variable_key[13];
	    		variables.decode_variable_request(variable_key, message);
	    		return variables.handle_variable_request(variable_key, message, channel, token, next_message_id(), descriptor.variable_type, descriptor.get_variable);
	    }
	    case CoAPMessageType::SAVE_BEGIN:
	      // fall through
	    case CoAPMessageType::UPDATE_BEGIN:
	        return chunkedTransfer.handle_update_begin(token, message, channel, callbacks.prepare_for_firmware_update, callbacks.millis);

	    case CoAPMessageType::CHUNK:
	        return chunkedTransfer.handle_chunk(token, message, channel, callbacks.save_firmware_chunk, callbacks.calculate_crc, callbacks.finish_firmware_update, callbacks.millis, [this]{return next_message_id();});

	    case CoAPMessageType::UPDATE_DONE:
	        return chunkedTransfer.handle_update_done(token, message, channel, callbacks.finish_firmware_update, [this]{return next_message_id();}, callbacks.millis);

	    case CoAPMessageType::EVENT:
	        return subscriptions.handle_event(message, descriptor.call_event_handler);

	    case CoAPMessageType::KEY_CHANGE:
	      // TODO
	      break;

	    case CoAPMessageType::SIGNAL_START:
	      message.set_length(Messages::coded_ack(message.buf(), token, ChunkReceivedCode::OK, queue[2], queue[3]));
	      callbacks.signal(true, 0, NULL);
	      return channel.send(message);

	    case CoAPMessageType::SIGNAL_STOP:
		  message.set_length(Messages::coded_ack(message.buf(), token, ChunkReceivedCode::OK, queue[2], queue[3]));
		  callbacks.signal(false, 0, NULL);
		  return channel.send(message);


	    case CoAPMessageType::HELLO:
	      descriptor.ota_upgrade_status_sent();
	      break;

	    case CoAPMessageType::TIME:
	      handle_time_response(queue[6] << 24 | queue[7] << 16 | queue[8] << 8 | queue[9]);
	      break;

	    case CoAPMessageType::PING:
	      message.set_length(Messages::empty_ack(message.buf(), queue[2], queue[3]));
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

	void handle_time_response(uint32_t time)
	{
	    // deduct latency
	    //uint32_t latency = last_chunk_millis ? (callbacks.millis()-last_chunk_millis)/2000 : 0;
	    //last_chunk_millis = 0;
	    // todo - compute connection latency
	    callbacks.set_time(time,0,NULL);
	}

public:

	Protocol(MessageChannel& channel_) : channel(channel_) {}

	/**
	 * Establish a secure connection and send and process the hello message.
	 */
	int begin()
	{
		ProtocolError error = channel.establish();
		if (error)
			return error;

		Message message;
		channel.create(message);
		size_t length = hello(descriptor.was_ota_upgrade_successful());
		message.set_length(length);
		error = channel.send(message);

		if (error)
		{
			ERROR("Hanshake: could not send hello message: %d", error);
			return error;
		}

		error = event_loop();        // read the hello message from the server
		if (error)
		{
			ERROR("Handshake: could not receive hello response");
			return error;
		}

		INFO("Hanshake: completed");
		return error;
	}


	// Returns true if no errors and still connected.
	// Returns false if there was an error, and we are probably disconnected.
	ProtocolError event_loop(void)
	{
		Message message;
		ProtocolError error = channel.receive(message);
		if (error) return error;

		if (message.length())
		{
			error = handle_received_message(message);
			if (error)
			{
				// bail if and only if there was an error
				chunkedTransfer.cancel(callbacks.finish_firmware_update);
				return error;
			}
		}
		else
		{
			error = event_loop_idle();
		}
		return error;
	}

};


}
}
