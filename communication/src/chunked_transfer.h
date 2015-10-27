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

#pragma once

#include "protocol_defs.h"
#include "message_channel.h"
#include "system_tick_hal.h"


namespace particle { namespace protocol {

class ChunkHandler
{
	system_tick_t last_chunk_millis;
	uint8_t updating;
	FileTransfer::Descriptor file;

protected:

	template<typename callback> bool handle_update_begin(Message& message, MessageChannel& channel,
			callback prepare_for_firmware_update)
	{
	    uint8_t flags = 0;
	    int actual_len = message.length();
	    uint8_t* queue = message.buf();
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
	    bool success = !prepare_for_firmware_update(file, 1, NULL);
	    if (success) {
	        success = file.chunk_count(file.chunk_size) < MAX_CHUNKS;
	    }
	    Message response;
	    channel.response(message);
	    coded_ack(response.buf(), success ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);

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




}}
