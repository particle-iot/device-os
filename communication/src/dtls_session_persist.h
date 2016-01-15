/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#define SessionPersistBaseSize 196

#include "mbedtls/ssl.h"
#include "stddef.h"

typedef struct __attribute__((packed)) SessionPersistOpaque
{
	uint16_t size;
//	uint8_t data[SessionPersistBaseSize-2+sizeof(mbedtls_ssl_session::ciphersuite)+sizeof(mbedtls_ssl_session::id_len)+sizeof(mbedtls_ssl_session::compression)];
	uint8_t data[SessionPersistBaseSize-2+sizeof(int)+sizeof(int)+sizeof(size_t)];
} SessionPersistOpaque;

#ifdef __cplusplus

#include "dtls_message_channel.h"
#include "spark_protocol_functions.h"	// for SparkCallbacks

namespace particle { namespace protocol {

struct __attribute__((packed)) SessionPersistData
{
	uint16_t size;

	/**
	 * non-zero if this context should be persisted when saved or updated.
	 *
	 */
	uint8_t persistent;

	uint8_t reserved;	// padding - use for something if needed.

	// do not add more members here - the offset of the public connection data should be
	// constant. Add more members at the end of the struct.
	uint8_t connection[32];
	uint32_t keys_checksum;
	uint8_t randbytes[sizeof(mbedtls_ssl_handshake_params::randbytes)];
	decltype(mbedtls_ssl_session::ciphersuite) ciphersuite;
	decltype(mbedtls_ssl_session::compression) compression;
	decltype(mbedtls_ssl_session::id_len) id_len;
	uint8_t id[sizeof(mbedtls_ssl_session::id)];
	uint8_t master[sizeof(mbedtls_ssl_session::master)];
	decltype(mbedtls_ssl_context::in_epoch) in_epoch;
	unsigned char out_ctr[8];
	// application data
	message_id_t next_coap_id;

};

class __attribute__((packed)) SessionPersist : SessionPersistData
{
public:

	using save_fn_t = decltype(DTLSMessageChannel::Callbacks::save);
	using restore_fn_t = decltype(DTLSMessageChannel::Callbacks::restore);

private:


	void restore_session(mbedtls_ssl_session* session)
	{
		session->ciphersuite = ciphersuite;
		session->compression = compression;
		session->id_len = id_len;
		memcpy(session->id, id, sizeof(id));
		memcpy(session->master, master, sizeof(master));
	}

	void save_session(mbedtls_ssl_session* session)
	{
		ciphersuite = session->ciphersuite;
		compression = session->compression;
		id_len = session->id_len;
		memcpy(id, session->id, sizeof(id));
		memcpy(master, session->master, sizeof(master));
	}

	/**
	 * Restores the context, even if the context itself is
	 * currently not flagged as persistent.
	 */
	bool restore_this_from(restore_fn_t restorer)
	{
		if (restorer)
		{
			if (restorer(this, sizeof(*this), SparkCallbacks::PERSIST_SESSION, nullptr)!=sizeof(*this))
				return false;
			if (size!=sizeof(*this))
				return false;
		}
		return true;
	}

	bool save_this_with(save_fn_t saver)
	{
		bool success = false;
		if (saver && persistent) {
			success = !saver(this, sizeof(*this), SparkCallbacks::PERSIST_SESSION, nullptr);
		}
		return success;
	}

public:

	SessionPersist()
	{
		size = 0; persistent = 0;
	}

	bool is_valid() { return size==sizeof(*this); }

	void invalidate() { size = 0; }

	void clear(save_fn_t saver)
	{
		persistent = 1;	// ensure it is saved
		invalidate();
		save_this_with(saver);
		persistent = 0;	// do not make any subsequent saves until the context is marked as persistent.
	}

	/**
	 * Prepare to transiently save information about this context.
	 */
	void prepare_save(const uint8_t* random, uint32_t keys_checksum, mbedtls_ssl_context* context, message_id_t next_id);

	/**
	 * Flags this context as being persistent. Subsequent calls
	 * to save and update will persist the state of this context.
	 * To remove persistence, call clear(), which clears the context, persists it,
	 * and clears the persistence flag.
	 */
	void make_persistent() { persistent = 1; }
	bool is_persistent() { return persistent; }

	/**
	 * Persist information in this context .
	 */
	void save(save_fn_t saver);

	/**
	 * Update information in this context and saves if the context
	 * is persistent.
	 */
	void update(mbedtls_ssl_context* context, save_fn_t saver, message_id_t next_id);

	enum RestoreStatus
	{
		/**
		 * Restoration is complete. No handshake is needed.
		 */
		COMPLETE,

		/**
		 * Restoration complete, a handshake is needed to complete.
		 */
		RENEGOTIATE,

		/**
		 * No session info was found. no changes were made to the
		 * ssl context.
		 */
		NO_SESSION,

		/**
		 * Could not restore. The session should be considered invalid.
		 */
		ERROR
	};

	/**
	 * Restores the state from this context. The persistence flag is not changed.
	 */
	RestoreStatus restore(mbedtls_ssl_context* context, bool renegotiate, uint32_t keys_checksum, message_id_t* message, restore_fn_t restorer);

	uint8_t* connection_data() { return connection; }

};

static_assert(sizeof(SessionPersist)==SessionPersistBaseSize+sizeof(mbedtls_ssl_session::ciphersuite)+sizeof(mbedtls_ssl_session::id_len)+sizeof(mbedtls_ssl_session::compression), "SessionPersist size");
static_assert(sizeof(SessionPersist)==sizeof(SessionPersistOpaque), "SessionPersistOpaque size == sizeof(SessionPersistQueue)");

// the connection buffer is used by external code to store connection data in the session
// it must be binary compatible with previous releases
static_assert(offsetof(SessionPersistData, connection)==4, "internal layout of public member has changed.");
static_assert(sizeof(SessionPersistData)==sizeof(SessionPersist), "session persist data and the subclass should be the same size.");

}}
#endif



