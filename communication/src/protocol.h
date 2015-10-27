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

class ChunkHandler
{
	system_tick_t last_chunk_millis;
	uint8_t updating;

protected:

	bool handle_update_begin(msg& message)
	{
	    // send ACK
	    uint8_t* msg_to_send = message.response;
	    *msg_to_send = 0;
	    *(msg_to_send + 1) = 16;

	    uint8_t flags = 0;
	    int actual_len = message.len - queue[message.len-1];
	    if (actual_len>=20 && queue[7]==0xFF) {
	        flags = decode_uint8(queue+8);
	        file.chunk_size = decode_uint16(queue+9);
	        file.file_length = decode_uint32(queue+11);
	        file.store = FileTransfer::Store::Enum(decode_uint8(queue+15));
	        file.file_address = decode_uint32(queue+16);
	        file.chunk_address = file.file_address;
	    }
	    else {
	        file.chunk_size = 0;
	        file.file_length = 0;
	        file.store = FileTransfer::Store::FIRMWARE;
	        file.file_address = 0;
	        file.chunk_address = 0;
	    }

	    // check the parameters only
	    bool success = !callbacks.prepare_for_firmware_update(file, 1, NULL);
	    if (success) {
	        success = file.chunk_count(file.chunk_size) < MAX_CHUNKS;
	    }

	    coded_ack(msg_to_send+2, success ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);
	    if (0 > blocking_send(msg_to_send, 18))
	    {
	      // error
	      return false;
	    }
	    if (success)
	    {
	        if (!callbacks.prepare_for_firmware_update(file, 0, NULL))
	        {
	            serial_dump("starting file length %d chunks %d chunk_size %d",
	                    file.file_length, file.chunk_count(file.chunk_size), file.chunk_size);
	            last_chunk_millis = callbacks.millis();
	            chunk_index = 0;
	            chunk_size = file.chunk_size;   // save chunk size since the descriptor size is overwritten
	            this->updating = 1;
	            // when not in fast OTA mode, the chunk missing buffer is set to 1 since the protocol
	            // handles missing chunks one by one. Also we don't know the actual size of the file to
	            // know the correct size of the bitmap.
	            set_chunks_received(flags & 1 ? 0 : 0xFF);

	            // send update_reaady - use fast OTA if available
	            update_ready(msg_to_send + 2, message.token, (flags & 0x1));
	            if (0 > blocking_send(msg_to_send, 18))
	            {
	              // error
	              return false;
	            }
	        }
	    }

	    return true;
	}

	bool handle_chunk(msg& message)
	{
	    last_chunk_millis = callbacks.millis();

	    uint8_t* msg_to_send = message.response;
	    // send ACK
	    *msg_to_send = 0;
	    *(msg_to_send + 1) = 16;
	    empty_ack(msg_to_send + 2, queue[2], queue[3]);
	    if (0 > blocking_send(msg_to_send, 18))
	    {
	      // error
	      return false;
	    }
	    serial_dump("chunk");
	    if (!this->updating) {
	        serial_dump("got chunk when not updating");
	        return true;
	    }

	    bool fast_ota = false;
	    uint8_t payload = 7;

	    unsigned option = 0;
	    uint32_t given_crc = 0;
	    while (queue[payload]!=0xFF) {
	        switch (option) {
	            case 0:
	                given_crc = decode_uint32(queue+payload+1);
	                break;
	            case 1:
	                this->chunk_index = decode_uint16(queue+payload+1);
	                fast_ota = true;
	                break;
	        }
	        option++;
	        payload += (queue[payload]&0xF)+1;  // increase by the size. todo handle > 11
	    }
	    if (fast_ota) {
	        doing_fast_ota();
	    }
	    if (0xFF==queue[payload])
	    {
	        payload++;
	        const uint8_t* chunk = queue+payload;
	        file.chunk_size = message.len - payload - queue[message.len - 1];   // remove length added due to pkcs #7 padding?
	        file.chunk_address  = file.file_address + (chunk_index * chunk_size);
	        if (chunk_index>=MAX_CHUNKS) {
	            serial_dump("invalid chunk index %d", chunk_index);
	            return false;
	        }
	        uint32_t crc = callbacks.calculate_crc(chunk, file.chunk_size);
	        bool has_response = false;
	        bool crc_valid = (crc == given_crc);
	        serial_dump("chunk idx=%d crc=%d fast=%d updating=%d", chunk_index, crc_valid, fast_ota, updating);
	        if (crc_valid)
	        {
	            callbacks.save_firmware_chunk(file, chunk, NULL);
	            if (!fast_ota || (updating!=2 && (true || (chunk_index & 32)==0))) {
	                chunk_received(msg_to_send + 2, message.token, ChunkReceivedCode::OK);
	                has_response = true;
	            }
	            flag_chunk_received(chunk_index);
	            if (updating==2) {                      // clearing up missed chunks at the end of fast OTA
	                chunk_index_t next_missed = next_chunk_missing(0);
	                if (next_missed==NO_CHUNKS_MISSING) {
	                    serial_dump("received all chunks");
	                    reset_updating();
	                    callbacks.finish_firmware_update(file, 1, NULL);
	                    notify_update_done(msg_to_send+2);
	                    has_response = true;
	                }
	                else {
	                    if (has_response && 0 > blocking_send(msg_to_send, 18)) {

	                        serial_dump("send chunk response failed");
	                        return false;
	                    }
	                    has_response = false;

	                    if (next_missed>missed_chunk_index)
	                        send_missing_chunks(MISSED_CHUNKS_TO_SEND);
	                }
	            }
	            chunk_index++;
	        }
	        else if (!fast_ota)
	        {
	            chunk_received(msg_to_send + 2, message.token, ChunkReceivedCode::BAD);
	            has_response = true;
	            serial_dump("chunk bad %d", chunk_index);
	        }
	        // fast OTA will request the chunk later

	        if (has_response && 0 > blocking_send(msg_to_send, 18))
	        {
	          // error
	          return false;
	        }
	    }

	    return true;
	}


	inline void flag_chunk_received(chunk_index_t idx)
	{
	//    serial_dump("flagged chunk %d", idx);
	    chunk_bitmap()[idx>>3] |= uint8_t(1<<(idx&7));
	}

	inline bool is_chunk_received(chunk_index_t idx)
	{
	    return (chunk_bitmap()[idx>>3] & uint8_t(1<<(idx&7)));
	}

	SparkProtocol::chunk_index_t next_chunk_missing(chunk_index_t start)
	{
	    chunk_index_t chunk = NO_CHUNKS_MISSING;
	    chunk_index_t chunks = file.chunk_count(chunk_size);
	    chunk_index_t idx = start;
	    for (;idx<chunks; idx++)
	    {
	        if (!is_chunk_received(idx))
	        {
	            //serial_dump("next missing chunk %d from %d", idx, start);
	            chunk = idx;
	            break;
	        }
	    }
	    return chunk;
	}

	void set_chunks_received(uint8_t value)
	{
	    size_t bytes = chunk_bitmap_size();
	    if (bytes)
	    		memset(queue+QUEUE_SIZE-bytes, value, bytes);
	}

	bool handle_update_done(msg& message)
	{
	    // send ACK 2.04
	    uint8_t* msg_to_send = message.response;

	    *msg_to_send = 0;
	    *(msg_to_send + 1) = 16;
	    serial_dump("update done received");
	    chunk_index_t index = next_chunk_missing(0);
	    bool missing = index!=NO_CHUNKS_MISSING;
	    coded_ack(msg_to_send + 2, message.token, missing ? ChunkReceivedCode::BAD : ChunkReceivedCode::OK, queue[2], queue[3]);
	    if (0 > blocking_send(msg_to_send, 18))
	    {
	        // error
	        return false;
	    }

	    if (!missing) {
	        serial_dump("update done - all done!");
	        reset_updating();
	        callbacks.finish_firmware_update(file, 1, NULL);
	    }
	    else {
	        updating = 2;       // flag that we are sending missing chunks.
	        serial_dump("update done - missing chunks starting at %d", index);
	        send_missing_chunks(MISSED_CHUNKS_TO_SEND);
	        last_chunk_millis = callbacks.millis();
	    }
	    return true;
	}


public:

	bool idle()
	{
		system_tick_t millis_since_last_chunk = callbacks.millis()
				- last_chunk_millis;
		if (3000 < millis_since_last_chunk)
		{
			if (updating == 2)
			{    // send missing chunks
				serial_dump("timeout - resending missing chunks");
				if (!send_missing_chunks(MISSED_CHUNKS_TO_SEND))
					return false;
			}
			/* Do not resend chunks since this can cause duplicates on the server.
			 else
			 {
			 queue[0] = 0;
			 queue[1] = 16;
			 chunk_missed(queue + 2, chunk_index);
			 if (0 > blocking_send(queue, 18))
			 {
			 // error
			 return false;
			 }
			 }
			 */
			last_chunk_millis = callbacks.millis();
		}
	}

	bool isUpdating()
	{
		return updating;
	}
};

class Events
{
	FilteringEventHandler event_handlers[5];

protected:
public:

	void handle_event(msg& message)
	{
	    const unsigned len = message.len;

	    // fist decode the event data before looking for a handler
	    unsigned char pad = queue[len - 1];
	    if (0 == pad || 16 < pad)
	    {
	        // ignore bad message, PKCS #7 padding must be 1-16
	        return;
	    }
	    // end of CoAP message
	    unsigned char *end = queue + len - pad;

	    unsigned char *event_name = queue + 6;
	    size_t event_name_length = CoAP::option_decode(&event_name);
	    if (0 == event_name_length)
	    {
	        // error, malformed CoAP option
	        return;
	    }

	    unsigned char *next_src = event_name + event_name_length;
	    unsigned char *next_dst = next_src;
	    while (next_src < end && 0x00 == (*next_src & 0xf0))
	    {
	      // there's another Uri-Path option, i.e., event name with slashes
	      size_t option_len = CoAP::option_decode(&next_src);
	      *next_dst++ = '/';
	      if (next_dst != next_src)
	      {
	        // at least one extra byte has been used to encode a CoAP Uri-Path option length
	        memmove(next_dst, next_src, option_len);
	      }
	      next_src += option_len;
	      next_dst += option_len;
	    }
	    event_name_length = next_dst - event_name;

	    if (next_src < end && 0x30 == (*next_src & 0xf0))
	    {
	      // Max-Age option is next, which we ignore
	      size_t next_len = CoAP::option_decode(&next_src);
	      next_src += next_len;
	    }

	    unsigned char *data = NULL;
	    if (next_src < end && 0xff == *next_src)
	    {
	      // payload is next
	      data = next_src + 1;
	      // null terminate data string
	      *end = 0;
	    }
	    // null terminate event name string
	    event_name[event_name_length] = 0;

	  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
	  for (int i = 0; i < NUM_HANDLERS; i++)
	  {
	    if (NULL == event_handlers[i].handler)
	    {
	       break;
	    }
	    const size_t MAX_FILTER_LENGTH = sizeof(event_handlers[i].filter);
	    const size_t filter_length = strnlen(event_handlers[i].filter, MAX_FILTER_LENGTH);

	    if (event_name_length < filter_length)
	    {
	      // does not match this filter, try the next event handler
	      continue;
	    }

	    const int cmp = memcmp(event_handlers[i].filter, event_name, filter_length);
	    if (0 == cmp)
	    {
	        // don't call the handler directly, use a callback for it.
	        if (!this->descriptor.call_event_handler)
	        {
	            if(event_handlers[i].handler_data)
	            {
	                EventHandlerWithData handler = (EventHandlerWithData) event_handlers[i].handler;
	                handler(event_handlers[i].handler_data, (char *)event_name, (char *)data);
	            }
	            else
	            {
	                event_handlers[i].handler((char *)event_name, (char *)data);
	            }
	        }
	        else
	        {
	            descriptor.call_event_handler(sizeof(FilteringEventHandler), &event_handlers[i], (const char*)event_name, (const char*)data, NULL);
	        }
	    }
	    // else continue the for loop to try the next handler
	  }
	}


	template <typename F> void for_each(F callback)
	{
		const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
		for (unsigned i = 0; i < NUM_HANDLERS; i++)
		{
		    if (nullptr != event_handlers[i].handler)
		    {
		    		callback(event_handlers[i]);
		    }
		}
	}

	void remove_event_handlers(const char* event_name)
	{
	    if (NULL == event_name)
	    {
	        memset(event_handlers, 0, sizeof(event_handlers));
	    }
	    else
	    {
	        const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
	        int dest = 0;
	        for (int i = 0; i < NUM_HANDLERS; i++)
	        {
	          if (!strcmp(event_name, event_handlers[i].filter))
	          {
	              memset(&event_handlers[i], 0, sizeof(event_handlers[i]));
	          }
	          else
	          {
	              if (dest!=i) {
	                memcpy(event_handlers+dest, event_handlers+i, sizeof(event_handlers[i]));
	                memset(event_handlers+i, 0, sizeof(event_handlers[i]));
	              }
	              dest++;
	          }
	        }
	    }
	}

	bool event_handler_exists(const char *event_name, EventHandler handler,
	    void *handler_data, SubscriptionScope::Enum scope, const char* id)
	{
	  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
	  for (int i = 0; i < NUM_HANDLERS; i++)
	  {
	      if (event_handlers[i].handler==handler &&
	          event_handlers[i].handler_data==handler_data &&
	          event_handlers[i].scope==scope) {
	        const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
	        const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
	        if (!strncmp(event_handlers[i].filter, event_name, FILTER_LEN)) {
	            const size_t MAX_ID_LEN = sizeof(event_handlers[i].device_id)-1;
	            const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
	            if (id_len)
	                return !strncmp(event_handlers[i].device_id, id, id_len);
	            else
	                return !event_handlers[i].device_id[0];
	        }
	      }
	  }
	  return false;
	}

	bool add_event_handler(const char *event_name, EventHandler handler,
	    void *handler_data, SubscriptionScope::Enum scope, const char* id)
	{
	    if (event_handler_exists(event_name, handler, handler_data, scope, id))
	        return true;

	  const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
	  for (int i = 0; i < NUM_HANDLERS; i++)
	  {
	    if (NULL == event_handlers[i].handler)
	    {
	      const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
	      const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
	      memcpy(event_handlers[i].filter, event_name, FILTER_LEN);
	      memset(event_handlers[i].filter + FILTER_LEN, 0, MAX_FILTER_LEN - FILTER_LEN);
	      event_handlers[i].handler = handler;
	      event_handlers[i].handler_data = handler_data;
	      event_handlers[i].device_id[0] = 0;
	        const size_t MAX_ID_LEN = sizeof(event_handlers[i].device_id)-1;
	        const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
	        memcpy(event_handlers[i].device_id, id, id_len);
	        event_handlers[i].device_id[id_len] = 0;
	        event_handlers[i].scope = scope;
	      return true;
	    }
	  }
	  return false;
	}
};

class Functions
{
	bool handle_function_call(msg& message)
	{
	    // copy the function key
	    char function_key[13];
	    memset(function_key, 0, 13);
	    int function_key_length = queue[7] & 0x0F;
	    memcpy(function_key, queue + 8, function_key_length);

	    // How long is the argument?
	    int q_index = 8 + function_key_length;
	    int query_length = queue[q_index] & 0x0F;
	    if (13 == query_length)
	    {
	      ++q_index;
	      query_length = 13 + queue[q_index];
	    }
	    else if (14 == query_length)
	    {
	      ++q_index;
	      query_length = queue[q_index] << 8;
	      ++q_index;
	      query_length |= queue[q_index];
	      query_length += 269;
	    }

	    bool has_function = false;

	    // allocated memory bounds check
	    if (MAX_FUNCTION_ARG_LENGTH > query_length)
	    {
	        // save a copy of the argument
	        memcpy(function_arg, queue + q_index + 1, query_length);
	        function_arg[query_length] = 0; // null terminate string
	        has_function = true;
	    }

	    uint8_t* msg_to_send = message.response;
	    // send ACK
	    msg_to_send[0] = 0;
	    msg_to_send[1] = 16;
	    coded_ack(msg_to_send + 2, has_function ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);
	    if (0 > blocking_send(msg_to_send, 18))
	    {
	      // error
	      return false;
	    }

	    // call the given user function
	    auto callback = [=] (const void* result, SparkReturnType::Enum resultType ) { return this->function_result(result, resultType, message.token); };
	    descriptor.call_function(function_key, function_arg, callback, NULL);
	    return true;
	}
};


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
