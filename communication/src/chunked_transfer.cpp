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

#include "chunked_transfer.h"
#include "service_debug.h"
#include "coap.h"

namespace particle { namespace protocol {

ProtocolError ChunkedTransfer::handle_update_begin(
		token_t token, Message& message, MessageChannel& channel)
{
	uint8_t flags = 0;
	int actual_len = message.length();
	uint8_t* queue = message.buf();
	message_id_t msg_id = CoAP::message_id(queue);
	if (actual_len >= 20 && queue[7] == 0xFF)
	{
		flags = decode_uint8(queue + 8);
		file.chunk_size = decode_uint16(queue + 9);
		file.file_length = decode_uint32(queue + 11);
		file.store = FileTransfer::Store::Enum(decode_uint8(queue + 15));
		file.file_address = decode_uint32(queue + 16);
		file.chunk_address = file.file_address;
	}
	else
	{
		file.chunk_size = 0;
		file.file_length = 0;
		file.store = FileTransfer::Store::FIRMWARE;
		file.file_address = 0;
		file.chunk_address = 0;
	}
	// check the parameters only
	bool success = !callbacks->prepare_for_firmware_update(file, 1, NULL);
	if (success)
	{
		success = file.chunk_count(file.chunk_size) < MAX_CHUNKS;
	}
	Message response;
	channel.response(message, response, 16);
	size_t size = Messages::coded_ack(response.buf(),
			success ? 0x00 : RESPONSE_CODE(4, 00), 0, 0);
	response.set_length(size);
	response.set_id(msg_id);
	ProtocolError error = channel.send(response);
	if (error)
		return error;

	if (success)
	{
		if (!callbacks->prepare_for_firmware_update(file, 0, NULL))
		{
			DEBUG("starting file length %d chunks %d chunk_size %d",
					file.file_length, file.chunk_count(file.chunk_size),
					file.chunk_size);
			last_chunk_millis = callbacks->millis();
			chunk_index = 0;
			chunk_size = file.chunk_size; // save chunk size since the descriptor size is overwritten
			updating = 1;
			Message updateReady;
			channel.create(updateReady);
			// updateReady will have the maximum capacity
			int offset = updateReady.capacity() - chunk_bitmap_size();
			bitmap = queue+offset; // this relies on the fact that we know the channels use a static buffer

			// when not in fast OTA mode, the chunk missing buffer is set to 1 since the protocol
			// handles missing chunks one by one. Also we don't know the actual size of the file to
			// know the correct size of the bitmap.
			set_chunks_received(flags & 1 ? 0 : 0xFF);

			// send update_reaady - use fast OTA if available
			size_t size = Messages::update_ready(updateReady.buf(), 0, token, (flags & 0x1), channel.is_unreliable());
			updateReady.set_length(size);
			error = channel.send(updateReady);
		}
	}
	return error;
}


ProtocolError ChunkedTransfer::handle_chunk(token_t token, Message& message,
		MessageChannel& channel)
{
	last_chunk_millis = callbacks->millis();

	Message response;
	ProtocolError error;
	channel.response(message, response, 16);	
	uint8_t* queue = message.buf();
	
	DEBUG("chunk");
	if (!this->updating)
	{
		WARN("got chunk when not updating");
		return INVALID_STATE;
	}

	bool fast_ota = false;
	uint8_t payload = 7;

	unsigned option = 0;
	uint32_t given_crc = 0;
	while (queue[payload] != 0xFF)
	{
		switch (option)
		{
		case 0:
			given_crc = decode_uint32(queue + payload + 1);
			break;
		case 1:
			this->chunk_index = decode_uint16(queue + payload + 1);
			fast_ota = true;
			break;
		}
		option++;
		payload += (queue[payload] & 0xF) + 1; // increase by the size. todo handle > 11
	}

	if (!fast_ota)
	{
		// send ACK
		message_id_t msg_id = CoAP::message_id(queue);
		size_t response_size = Messages::empty_ack(response.buf(), 0, 0);
		response.set_id(msg_id);
		response.set_length(response_size);
		error = channel.send(response);
		if (error)
			return error;
	}

	channel.create(response);

	if (0xFF == queue[payload])
	{
		payload++;
		const uint8_t* chunk = queue + payload;
		file.chunk_size = message.length() - payload;
		file.chunk_address = file.file_address + (chunk_index * chunk_size);
		if (chunk_index >= MAX_CHUNKS)
		{
			WARN("invalid chunk index %d", chunk_index);
			return NO_ERROR;
		}
		uint32_t crc = callbacks->calculate_crc(chunk, file.chunk_size);
		uint16_t response_size = 0;
		bool crc_valid = (crc == given_crc);
		DEBUG("chunk idx=%d crc=%d fast=%d updating=%d", chunk_index,
				crc_valid, fast_ota, updating);
		if (crc_valid)
		{
			callbacks->save_firmware_chunk(file, chunk, NULL);
			if (!fast_ota)
			{
				// message is confirmable for regular OTA or when
				response_size = Messages::chunk_received(response.buf(), 0, token, ChunkReceivedCode::OK, channel.is_unreliable());
			}
			flag_chunk_received(chunk_index);
			if (updating == 2)
			{            // clearing up missed chunks at the end of fast OTA
				chunk_index_t next_missed = next_chunk_missing(0);
				if (next_missed == NO_CHUNKS_MISSING)
				{
					INFO("received all chunks");
					reset_updating();
					callbacks->finish_firmware_update(file, 1, NULL);
					response_size = Messages::update_done(response.buf(), 0, channel.is_unreliable());
				}
				else
				{
					if (response_size)
					{
						response.set_length(response_size);
						error = channel.send(response);
						response_size = 0;
						if (error)
						{
							WARN("send chunk response failed");
							return error;
						}
					}
					if (next_missed > missed_chunk_index)
						send_missing_chunks(message, channel, MISSED_CHUNKS_TO_SEND);
				}
			}
			chunk_index++;
		}
		else
		{
			WARN("chunk crc bad %d: wanted %x got %x", chunk_index, given_crc, crc);
			if (!fast_ota)
			{
				response_size = Messages::chunk_received(response.buf(), 0, token, ChunkReceivedCode::BAD, channel.is_unreliable());
			}
			// fast OTA will request the chunk later
		}
		if (response_size)
		{
			response.set_length(response_size);
			error = channel.send(response);
			if (error)
				return error;
		}
	}
	return NO_ERROR;
}

ProtocolError ChunkedTransfer::handle_update_done(token_t token, Message& message,
		MessageChannel& channel)
{
	// send ACK 2.04
	Message response;
	channel.response(message, response, 16);

	DEBUG("update done received");
	chunk_index_t index = next_chunk_missing(0);
	bool missing = index != NO_CHUNKS_MISSING;
	uint8_t* queue = message.buf();
	message_id_t msg_id = CoAP::message_id(queue);
	size_t response_size = Messages::coded_ack(response.buf(), token,
			missing ? ChunkReceivedCode::BAD : ChunkReceivedCode::OK,
			0, 0);
	response.set_length(response_size);
	response.set_id(msg_id);
	ProtocolError error = channel.send(response);
	if (error)
		return error;

	if (!missing)
	{
		DEBUG("update done - all done!");
		reset_updating();
		callbacks->finish_firmware_update(file, 1, NULL);
	}
	else
	{
		updating = 2;       // flag that we are sending missing chunks.
		DEBUG("update done - missing chunks starting at %d", index);
		error = send_missing_chunks(message, channel, MISSED_CHUNKS_TO_SEND);
		last_chunk_millis = callbacks->millis();
	}
	return error;
}

ProtocolError ChunkedTransfer::send_missing_chunks(Message& message, MessageChannel& channel,
		size_t count)
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

	while ((idx = next_chunk_missing(chunk_index_t(idx)))
			!= NO_CHUNKS_MISSING && sent < count)
	{
		buf[(sent * 2) + 7] = idx >> 8;
		buf[(sent * 2) + 8] = idx & 0xFF;

		missed_chunk_index = idx;
		idx++;
		sent++;
	}

	if (sent > 0)
	{
		DEBUG("Sent %d missing chunks", sent);
		size_t message_size = 7 + (sent * 2);
		message.set_length(message_size);
		ProtocolError error = channel.send(message);
		if (error)
			return error;
	}
	return NO_ERROR;
}

ProtocolError ChunkedTransfer::idle(MessageChannel& channel)
{
	system_tick_t millis_since_last_chunk = callbacks->millis() - last_chunk_millis;
	if (3000 < millis_since_last_chunk)
	{
		if (updating == 2)
		{    // send missing chunks
			WARN("timeout - resending missing chunks");
			Message message;
			ProtocolError error = channel.create(message,
					MISSED_CHUNKS_TO_SEND * sizeof(chunk_index_t) + 7);
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
		last_chunk_millis = callbacks->millis();
	}
	return NO_ERROR;
}

void ChunkedTransfer::cancel()
{
	if (is_updating())
	{
		// was updating but had an error, inform the client
		WARN("handle received message failed - aborting transfer");
		callbacks->finish_firmware_update(file, 0, NULL);
	}
}


chunk_index_t ChunkedTransfer::next_chunk_missing(chunk_index_t start)
{
	chunk_index_t chunk = NO_CHUNKS_MISSING;
	chunk_index_t chunks = file.chunk_count(chunk_size);
	chunk_index_t idx = start;
	for (; idx < chunks; idx++)
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

void ChunkedTransfer::set_chunks_received(uint8_t value)
{
	size_t bytes = chunk_bitmap_size();
	if (bytes)
		memset(bitmap, value, bytes);
}


}}
