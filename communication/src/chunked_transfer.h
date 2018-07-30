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

namespace particle
{
namespace protocol
{

class ChunkedTransfer
{

public:

	/**
	 * Callbacks interface for chunk transfer.
	 * This is coded as a abstract virtual interface so that the vtable
	 * exists in flash. This also allows tests using mocking with fakeit.
	 */
	struct Callbacks
	{
		  /**
		   * @param flags 1 dry run only.
		   * Return 0 on success.
		   */
		  virtual int prepare_for_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void*)=0;

		  /**
		   *
		   * @return 0 on success
		   */
		  virtual int save_firmware_chunk(FileTransfer::Descriptor& descriptor, const unsigned char* chunk, void*)=0;

		  /**
		   * Finalize the data storage.
		   * #param reset - if the device should be reset to apply the changes.
		   * #return 0 on success. Other values indicate an issue with the file.
		   */
		  virtual int finish_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void*)=0;

		  virtual uint32_t calculate_crc(const unsigned char *buf, uint32_t buflen)=0;

		  virtual system_tick_t millis()=0;
	};

private:
	uint8_t updating;
	system_tick_t last_chunk_millis;
	FileTransfer::Descriptor file;

	/**
	 * Marks the indices of missed chunks not yet requested.
	 */
	chunk_index_t missed_chunk_index;
	/**
	 * Number of chunks received in the current flight of chunks (between UpdateBegin|UpdateDone and UpdateDone)
	 */
	chunk_index_t chunk_count;

	unsigned short chunk_index;
	unsigned short chunk_size;

	uint8_t* bitmap;

	Callbacks* callbacks;

	bool fast_ota_override;
	bool fast_ota_value;

protected:

	unsigned chunk_bitmap_size()
	{
		return (file.chunk_count(chunk_size) + 7) / 8;
	}

	uint8_t* chunk_bitmap()
	{
		return bitmap;
	}

	inline void flag_chunk_received(chunk_index_t idx)
	{
		//    serial_dump("flagged chunk %d", idx);
		chunk_bitmap()[idx >> 3] |= uint8_t(1 << (idx & 7));
	}

	inline bool is_chunk_received(chunk_index_t idx)
	{
		return (chunk_bitmap()[idx >> 3] & uint8_t(1 << (idx & 7)));
	}

	chunk_index_t next_chunk_missing(chunk_index_t start);
	void set_chunks_received(uint8_t value);
public:

	ChunkedTransfer() :
			updating(false), callbacks(nullptr), fast_ota_override(false), fast_ota_value(true)
	{
	}

	void init(Callbacks* callbacks)
	{
		this->callbacks = callbacks;
	}

	void reset()
	{
		reset_updating();
		bitmap = nullptr;
		last_chunk_millis = 0;
	}

	ProtocolError handle_update_begin(token_t token, Message& message, MessageChannel& channel);

	ProtocolError handle_chunk(token_t token, Message& message, MessageChannel& channel);

	ProtocolError handle_update_done(token_t token, Message& message, MessageChannel& channel);

	ProtocolError send_missing_chunks(MessageChannel& channel, size_t count);

	ProtocolError idle(MessageChannel& channel);

	void set_fast_ota(unsigned data)
	{
		fast_ota_value = (data > 0) ? true : false;
		fast_ota_override = true;
	}

	bool is_updating()
	{
		return updating;
	}

	void reset_updating(void)
	{
		updating = false;
		last_chunk_millis = 0;    // this is used for the time latency also
	}

	void cancel();

	size_t notify_update_done(Message& msg, Message& response, MessageChannel& channel, token_t token,
							  uint8_t code);

};

}
}
