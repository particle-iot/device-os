#pragma once

#include "message_channel.h"
#include "service_debug.h"
#include "protocol_defs.h"
#include "ping.h"



namespace particle
{
namespace protocol
{


#if 0



class Protocol
{
	MessageChannel& channel;
	system_tick_t last_message_millis;

protected:

	void hello()
	{
		unsigned short message_id = next_message_id();
		bool newly_upgraded = descriptor.was_ota_upgrade_successful();

		Message msg = channel.message();
		size_t len = Messages::hello(message.buf(), newly_upgraded, PLATFORM_ID,
				product_id, product_firmware_version);
		msg.send();
	}

	bool event_loop_idle()
	{
		if (chunkHandler.isUpdating())
		{
			return chunkHandler.idle();
		}
		else
		{
			if (!pingHandler.idle())
				return false;
		}
		return true;
	}

	void sent_subscription(const FilterEventHHandler& handler)
	{
	    uint16_t msg_id = next_message_id();
	    size_t msglen;
	    Message msg = channel.message();
        if (handler.device_id[0])
       	  msglen = Messages::subscription(msg.buf(), msg_id, handler.filter, handler.device_id);
        else
           msglen = Messages::subscription(msg.buf(), msg_id, handler.filter, handler.scope);
        msg.send(msglen);
	}

	const int MISSED_CHUNKS_TO_SEND = 50;

	void send_missing_chunks()
	{
	    int sent = 0;
	    chunk_index_t idx = 0;

	    Message msg = channel.message();

	    uint8_t* buf = queue+2;
	    unsigned short message_id = next_message_id();
	    buf[0] = 0x40; // confirmable, no token
	    buf[1] = 0x01; // code 0.01 GET
	    buf[2] = message_id >> 8;
	    buf[3] = message_id & 0xff;
	    buf[4] = 0xb1; // one-byte Uri-Path option
	    buf[5] = 'c';
	    buf[6] = 0xff; // payload marker

	    while ((idx=next_chunk_missing(chunk_index_t(idx)))!=NO_CHUNKS_MISSING && sent<count)
	    {
	        buf[(sent*2)+7] = idx >> 8;
	        buf[(sent*2)+8] = idx & 0xFF;

	        missed_chunk_index = idx;
	        idx++;
	        sent++;
	    }

	    if (sent>0) {
	        serial_dump("Sent %d missing chunks", sent);
	        size_t message_size = 7+(sent*2);
	        msg.send(message_size);
	    }
	    return sent;
	}

	int description(unsigned char token, int desc_flags)
	{
		uint16_t message_id = next_message_id();
		Message msg = channel.message();

		size_t desc = Messages::description(buf, message_id, token);

		BufferAppender appender(buf+desc, msg.max_size()-8);
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
	    return msg.send(msglen);
	}

	bool handle_received_message(Message& request)
	{
	  last_message_millis = callbacks.millis();
	  expecting_ping_ack = false;
	  uint8_t* queue = request.buf();
	  CoAPMessageType::Enum message_type = received_message(queue, request.length());

	  unsigned char token = queue[4];
	  unsigned char *msg_to_send = queue + len;

	  Message message;
	  message.len = len;
	  message.token = queue[4];
	  message.response = msg_to_send;
	  message.response_len = request.max_size()-len;

	  switch (message_type)
	  {
	    case CoAPMessageType::DESCRIBE:
	    {
	        if (!send_description(DESCRIBE_SYSTEM, message) || !send_description(DESCRIBE_APPLICATION, message)) {
	            return false;
	        }
	        break;
	    }
	    case CoAPMessageType::FUNCTION_CALL:
	        if (!handle_function_call(message))
	            return false;
	        break;
	    case CoAPMessageType::VARIABLE_REQUEST:
	    {
	      // copy the variable key
	      int variable_key_length = queue[7] & 0x0F;
	      if (12 < variable_key_length)
	        variable_key_length = 12;

	      char variable_key[13];
	      memcpy(variable_key, queue + 8, variable_key_length);
	      memset(variable_key + variable_key_length, 0, 13 - variable_key_length);

	      queue[0] = 0;
	      queue[1] = 16; // default buffer length

	      // get variable value according to type using the descriptor
	      SparkReturnType::Enum var_type = descriptor.variable_type(variable_key);
	      if(SparkReturnType::BOOLEAN == var_type)
	      {
	        bool *bool_val = (bool *)descriptor.get_variable(variable_key);
	        variable_value(queue + 2, token, queue[2], queue[3], *bool_val);
	      }
	      else if(SparkReturnType::INT == var_type)
	      {
	        int *int_val = (int *)descriptor.get_variable(variable_key);
	        variable_value(queue + 2, token, queue[2], queue[3], *int_val);
	      }
	      else if(SparkReturnType::STRING == var_type)
	      {
	        char *str_val = (char *)descriptor.get_variable(variable_key);

	        // 2-byte leading length, 16 potential padding bytes
	        int max_length = QUEUE_SIZE - 2 - 16;
	        int str_length = strlen(str_val);
	        if (str_length > max_length) {
	          str_length = max_length;
	        }

	        int buf_size = variable_value(queue + 2, token, queue[2], queue[3], str_val, str_length);
	        queue[1] = buf_size & 0xff;
	        queue[0] = (buf_size >> 8) & 0xff;
	      }
	      else if(SparkReturnType::DOUBLE == var_type)
	      {
	        double *double_val = (double *)descriptor.get_variable(variable_key);
	        variable_value(queue + 2, token, queue[2], queue[3], *double_val);
	      }

	      // buffer length may have changed if variable is a long string
	      if (0 > blocking_send(queue, (queue[0] << 8) + queue[1] + 2))
	      {
	        // error
	        return false;
	      }
	      break;
	    }

	    case CoAPMessageType::SAVE_BEGIN:
	      // fall through
	    case CoAPMessageType::UPDATE_BEGIN:
	        return handle_update_begin(message);

	    case CoAPMessageType::CHUNK:
	        return handle_chunk(message);

	    case CoAPMessageType::UPDATE_DONE:
	        return handle_update_done(message);

	    case CoAPMessageType::EVENT:
	        handle_event(message);
	          break;
	    case CoAPMessageType::KEY_CHANGE:
	      // TODO
	      break;
	    case CoAPMessageType::SIGNAL_START:
	      queue[0] = 0;
	      queue[1] = 16;
	      coded_ack(queue + 2, token, ChunkReceivedCode::OK, queue[2], queue[3]);
	      if (0 > blocking_send(queue, 18))
	      {
	        // error
	        return false;
	      }

	      callbacks.signal(true, 0, NULL);
	      break;
	    case CoAPMessageType::SIGNAL_STOP:
	      queue[0] = 0;
	      queue[1] = 16;
	      coded_ack(queue + 2, token, ChunkReceivedCode::OK, queue[2], queue[3]);
	      if (0 > blocking_send(queue, 18))
	      {
	        // error
	        return false;
	      }

	      callbacks.signal(false, 0, NULL);
	      break;

	    case CoAPMessageType::HELLO:
	      descriptor.ota_upgrade_status_sent();
	      break;

	    case CoAPMessageType::TIME:
	      handle_time_response(queue[6] << 24 | queue[7] << 16 | queue[8] << 8 | queue[9]);
	      break;

	    case CoAPMessageType::PING:
	      *msg_to_send = 0;
	      *(msg_to_send + 1) = 16;
	      empty_ack(msg_to_send + 2, queue[2], queue[3]);
	      if (0 > blocking_send(msg_to_send, 18))
	      {
	        // error
	        return false;
	      }
	      break;

	    case CoAPMessageType::EMPTY_ACK:
	    case CoAPMessageType::ERROR:
	    default:
	      ; // drop it on the floor
	  }

	  // all's well
	  return true;
	}

	void handle_time_response(uint32_t time)
	{
	    // deduct latency
	    uint32_t latency = last_chunk_millis ? (callbacks.millis()-last_chunk_millis)/2000 : 0;
	    last_chunk_millis = 0;
	    callbacks.set_time(time-latency,0,NULL);
	}


public:
	/**
	 * Establish a secure connection and send and process the hello message.
	 */
	int begin()
	{
		int error = channel.handshake();
		if (!error)
			return error;

		message_hello();

		err = channel.blocking_send(queue, 18);
		if (0 > err)
		{
			ERROR("Hanshake: could not send hello message: %d", err);
			return err;
		}

		if (!event_loop())        // read the hello message from the server
		{
			ERROR("Handshake: could not receive hello response");
			return -1;
		}
		INFO("Hanshake: completed");
		return 0;

	}


	// Returns true if no errors and still connected.
	// Returns false if there was an error, and we are probably disconnected.
	bool event_loop(void)
	{
		int bytes_received = callbacks.receive(queue, 2);
		if (2 <= bytes_received)
		{
			bool success = handle_received_message();
			if (!success)
			{
				if (updating)
				{      // was updating but had an error, inform the client
					serial_dump("handle received message failed - aborting transfer");
					callbacks.finish_firmware_update(file, 0, NULL);
					updating = false;
				}

				// bail if and only if there was an error
				return false;
			}
		}
		else
		{
			if (0 > bytes_received)
			{
				// error, disconnected
				return false;
			}

			event_loop_idle();
		}

		// no errors, still connected
		return true;
	}


}

#endif

}
}
