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
#include "file_transfer.h"
#include "message_channel.h"
#include "system_tick_hal.h"
#include "messages.h"

namespace particle { namespace protocol {

class ChunkedTransfer
{
	system_tick_t last_chunk_millis;
	uint8_t updating;
	FileTransfer::Descriptor file;

    /**
     * Marks the indices of missed chunks not yet requested.
     */
    chunk_index_t missed_chunk_index;
    unsigned short chunk_index;
    unsigned short chunk_size;

    uint8_t* bitmap;

protected:

    void reset_updating(void)
    {
      updating = false;
      last_chunk_millis = 0;    // this is used for the time latency also
    }

    unsigned chunk_bitmap_size()
    {
        return (file.chunk_count(chunk_size)+7)/8;
    }

    uint8_t* chunk_bitmap()
    {
        return bitmap;
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

	chunk_index_t next_chunk_missing(chunk_index_t start)
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
	    		memset(bitmap, value, bytes);
	}

public:
	ChunkedTransfer() : updating(false) {}

	template<typename callback_prepare, typename callback_millis> ProtocolError
	handle_update_begin(token_t token, Message& message, MessageChannel& channel,
			callback_prepare prepare_for_firmware_update, callback_millis millis)
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
	    channel.response(message, response, 16);
	    size_t size = Messages::coded_ack(response.buf(), success ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);
	    response.set_length(size);
	    ProtocolError error = channel.send(response);
	    if (error) return error;
	    bitmap = &queue[message.capacity()-chunk_bitmap_size()]; 	// this relies on the fact that we know the channels use a static buffer


	    if (success)
	    {
	        if (!prepare_for_firmware_update(file, 0, NULL))
	        {
	            DEBUG("starting file length %d chunks %d chunk_size %d",
	                    file.file_length, file.chunk_count(file.chunk_size), file.chunk_size);
	            last_chunk_millis = millis();
	            chunk_index = 0;
	            chunk_size = file.chunk_size;   // save chunk size since the descriptor size is overwritten
	            updating = 1;
	            // when not in fast OTA mode, the chunk missing buffer is set to 1 since the protocol
	            // handles missing chunks one by one. Also we don't know the actual size of the file to
	            // know the correct size of the bitmap.
	            set_chunks_received(flags & 1 ? 0 : 0xFF);

	            // send update_reaady - use fast OTA if available
	            size_t size = Messages::update_ready(response.buf(), token, (flags & 0x1));
	            response.set_length(size);
	            error = channel.send(response);
	        }
	    }

	    return error;
	}

	template<typename callback_save_firmware_chunk, typename callback_calculate_crc, typename callback_finish_firmware_update, typename callback_millis>
	ProtocolError handle_chunk(token_t token, Message& message, MessageChannel& channel, callback_save_firmware_chunk save_firmware_chunk,
				callback_calculate_crc calculate_crc, callback_finish_firmware_update finish_firmware_update, callback_millis millis)
	{
	    last_chunk_millis = millis();

	    Message response;
	    channel.response(message, response, 16);
	    // send ACK
	    uint8_t* queue = message.buf();
	    size_t response_size = Messages::empty_ack(response.buf(), queue[2], queue[3]);
	    response.set_length(response_size);
	    ProtocolError error = channel.send(response);
	    if (error) return error;

	    DEBUG("chunk");
	    if (!this->updating) {
	        WARN("got chunk when not updating");
	        return INVALID_STATE;
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
	    if (0xFF==queue[payload])
	    {
	        payload++;
	        const uint8_t* chunk = queue+payload;
	        file.chunk_size = message.length() - payload;
	        file.chunk_address  = file.file_address + (chunk_index * chunk_size);
	        if (chunk_index>=MAX_CHUNKS) {
	            WARN("invalid chunk index %d", chunk_index);
	            return NO_ERROR;
	        }
	        uint32_t crc = calculate_crc(chunk, file.chunk_size);
	        uint16_t response_size = 0;
	        bool crc_valid = (crc == given_crc);
	        DEBUG("chunk idx=%d crc=%d fast=%d updating=%d", chunk_index, crc_valid, fast_ota, updating);
	        if (crc_valid)
	        {
	            save_firmware_chunk(file, chunk, NULL);
	            if (!fast_ota || (updating!=2 && ((chunk_index & 32)==0))) {
	                response_size = Messages::chunk_received(response.buf(), 0, token, ChunkReceivedCode::OK);
	            }
	            flag_chunk_received(chunk_index);
	            if (updating==2) {                      // clearing up missed chunks at the end of fast OTA
	                chunk_index_t next_missed = next_chunk_missing(0);
	                if (next_missed==NO_CHUNKS_MISSING) {
	                    INFO("received all chunks");
	                    reset_updating();
	                    finish_firmware_update(file, 1, NULL);
	                    response_size = Messages::update_done(response.buf(), 0);
	                }
	                else {
	                		if (response_size) {
	                			response.set_length(response_size);
	                			error = channel.send(response);
	                			response_size = 0;
	                		    if (error) {
	                		    		WARN("send chunk response failed");
	                		    		return error;
	                		    }
	                		}
	                    if (next_missed>missed_chunk_index)
	                    		send_missing_chunks(message, channel, MISSED_CHUNKS_TO_SEND);
	                }
	            }
	            chunk_index++;
	        }
	        else if (!fast_ota)
	        {
	            response_size = Messages::chunk_received(response.buf(), 0, token, ChunkReceivedCode::BAD);
	            WARN("chunk bad %d", chunk_index);
	        }
	        // fast OTA will request the chunk later

	        if (response_size)
	        {
	        		response.set_length(response_size);
	        		error = channel.send(response);
	        		if (error) return error;
	        }
	    }
	    return NO_ERROR;
	}



	template <typename callback_finish_firmware_update, typename callback_millis>
	ProtocolError handle_update_done(token_t token, Message& message, MessageChannel& channel, callback_finish_firmware_update finish_firmware_update,
			callback_millis millis)
	{
	    // send ACK 2.04
		Message response;
		channel.response(message, response, 16);

	    DEBUG("update done received");
	    chunk_index_t index = next_chunk_missing(0);
	    bool missing = index!=NO_CHUNKS_MISSING;
	    uint8_t* queue = message.buf();
	    size_t response_size = Messages::coded_ack(response.buf(), token, missing ? ChunkReceivedCode::BAD : ChunkReceivedCode::OK, queue[2], queue[3]);
	    response.set_length(response_size);
	    ProtocolError error = channel.send(response);
	    if (error) return error;

	    if (!missing) {
	        DEBUG("update done - all done!");
	        reset_updating();
	        finish_firmware_update(file, 1, NULL);
	    }
	    else {
	        updating = 2;       // flag that we are sending missing chunks.
	        DEBUG("update done - missing chunks starting at %d", index);
	        error = send_missing_chunks(message, channel, MISSED_CHUNKS_TO_SEND);
	        last_chunk_millis = millis();
	    }
	    return error;
	}

	ProtocolError send_missing_chunks(Message& message, MessageChannel& channel, size_t count)
	{
		size_t sent = 0;
	    chunk_index_t idx = 0;
	    uint8_t* buf = message.buf();
	    buf[0] = 0x40; // confirmable, no token
	    buf[1] = 0x01; // code 0.01 GET
	    buf[2] = 0;
	    buf[3] = 0;
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
	        DEBUG("Sent %d missing chunks", sent);
	        size_t message_size = 7+(sent*2);
	        message.set_length(message_size);
	        ProtocolError error = channel.send(message);
	        if (error) return error;
	    }
	    return NO_ERROR;
	}

	template <typename callback_millis>
	inline ProtocolError idle(MessageChannel& channel, callback_millis millis)
	{
		system_tick_t millis_since_last_chunk = millis() - last_chunk_millis;
		if (3000 < millis_since_last_chunk)
		{
			if (updating == 2)
			{    // send missing chunks
				WARN("timeout - resending missing chunks");
				Message message;
				ProtocolError error = channel.create(message, MISSED_CHUNKS_TO_SEND*sizeof(chunk_index_t)+7);
				if (!error)
					error = send_missing_chunks(message, channel, MISSED_CHUNKS_TO_SEND);
				if (error)
					return error;
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
			last_chunk_millis = millis();
		}
		return NO_ERROR;
	}

	bool is_updating()
	{
		return updating;
	}

	template <typename callback_finish_firmware_update> void cancel(callback_finish_firmware_update finish)
	{
		if (is_updating())
		{
			// was updating but had an error, inform the client
			WARN("handle received message failed - aborting transfer");
			finish(file, 0, NULL);
		}
	}
};




}}
