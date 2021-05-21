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

#include "stddef.h"

// The size of the persisted data
#define SessionPersistBaseSize 250

// variable size due to int/size_t members
#define SessionPersistVariableSize (sizeof(int)+sizeof(int)+sizeof(size_t))

/**
 * An entirely opaque version of SessionPersistData for use with C.
 */
typedef struct __attribute__((packed)) SessionPersistDataOpaque
{
	uint16_t size;
//	uint8_t data[SessionPersistBaseSize-sizeof(uint16_t)+sizeof(mbedtls_ssl_session::ciphersuite)+sizeof(mbedtls_ssl_session::id_len)+sizeof(mbedtls_ssl_session::compression)];
	uint8_t data[SessionPersistBaseSize-sizeof(uint16_t)+SessionPersistVariableSize];
} SessionPersistDataOpaque;



#ifdef __cplusplus
#include "coap.h"
#include "spark_protocol_functions.h"	// for SparkCallbacks

#ifndef NO_MBEDTLS_HEADERS
#include "dtls_message_channel.h"
#endif // NO_MBEDTLS_HEADERS


namespace particle { namespace protocol {

/**
 * Size of a DTLS connection ID.
 */
const size_t DTLS_CID_SIZE = 8;

#ifdef MBEDTLS_SSL_H
/**
 * A simple POD for the persisted session data.
 */
struct __attribute__((packed)) SessionPersistData
{
	uint16_t size;

	/**
	 * non-zero if this context should be persisted when saved or updated.
	 *
	 */
	uint8_t persistent;

	uint8_t use_counter;	    // the number of times this session has been retrieved without being successfully used.

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

	/**
	 * Checksum of the state of the subscriptions that have been sent to the cloud.
	 */
	uint32_t subscriptions_crc;
	/**
	 * Checksum of state of the functions and variables last sent to the cloud.
	 */
	uint32_t describe_app_crc;
	/**
	 * Checksum of the system describe message.
	 */
	uint32_t describe_system_crc;
	/**
	 * Protocol flags.
	 */
	uint32_t protocol_flags;
	/**
	 * Application state flags (see the `AppStateDescriptor::StateFlag` enum).
	 */
	uint32_t app_state_flags;
	/**
	 * Maximum size of a firmware binary.
	 */
	uint32_t max_binary_size;
	/**
	 * Module version of the system firmware.
	 */
	uint16_t system_version;
	/**
	 * Maximum size of a CoAP message.
	 */
	uint16_t max_message_size;
	/**
	 * Size of an OTA update chunk.
	 */
	uint16_t ota_chunk_size;
	/**
	 * Last validated sequence number of an incoming record.
	 */
	uint64_t in_window_top;
	/**
	 * Bitmap of last N incoming records.
	 */
	uint64_t in_window;
	/**
	 * Connection ID.
	 */
	uint8_t cid[DTLS_CID_SIZE];
};

class __attribute__((packed)) SessionPersistOpaque : public SessionPersistData
{
public:

	SessionPersistOpaque() {
		invalidate();
	}

	bool is_valid() { return size==sizeof(*this); }

	uint8_t* connection_data() { return connection; }

	void invalidate() {
		memset(this, 0, sizeof(*this));
	}

	void increment_use_count() { use_counter++; }
	void clear_use_count() { use_counter = 0; }
	int use_count() { return use_counter; }
	bool has_expired() { return use_counter >= MAXIMUM_SESSION_USES; }

	static const int MAXIMUM_SESSION_USES = 3;
};

class __attribute__((packed)) SessionPersist : public SessionPersistOpaque
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
		if (!restorer) {
			return false;
		}
		const int size = restorer(this, sizeof(*this), SparkCallbacks::PERSIST_SESSION, nullptr);
		if (size != sizeof(*this)) {
			DEBUG("restore size mismatch 1: %d/%d", size, sizeof(*this));
			return false;
		}
		if (this->size != sizeof(*this)) {
			DEBUG("restore size mismatch 2: %d/%d", (int)this->size, sizeof(*this));
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

	void clear(save_fn_t saver)
	{
		invalidate();
		persistent = 1;	// ensure it is saved
		save_this_with(saver);
		persistent = 0;	// do not make any subsequent saves until the context is marked as persistent.
	}

	/**
	 * Prepare to transiently save information about this context.
	 */
	bool prepare_save(const uint8_t* random, uint32_t keys_checksum, mbedtls_ssl_context* context, message_id_t next_id);

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
	void save(save_fn_t saver) { save_this_with(saver); };
	void restore(restore_fn_t restore) { restore_this_from(restore); }

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
	RestoreStatus restore(mbedtls_ssl_context* context, bool renegotiate, uint32_t keys_checksum, message_id_t* message, restore_fn_t restorer, save_fn_t saver);

	AppStateDescriptor app_state_descriptor();

	SessionPersistData& as_data() { return *this; }
};

static_assert(sizeof(SessionPersist)==SessionPersistBaseSize+sizeof(mbedtls_ssl_session::ciphersuite)+sizeof(mbedtls_ssl_session::id_len)+sizeof(mbedtls_ssl_session::compression), "SessionPersist size");
static_assert(sizeof(SessionPersist)==sizeof(SessionPersistDataOpaque), "SessionPersistDataOpaque size == sizeof(SessionPersist)");

static_assert(DTLS_CID_SIZE <= MBEDTLS_SSL_CID_OUT_LEN_MAX, "DTLS_CID_SIZE is too large");

// the connection buffer is used by external code to store connection data in the session
// it must be binary compatible with previous releases
static_assert(offsetof(SessionPersistData, connection)==4, "internal layout of public member has changed.");
static_assert((sizeof(SessionPersistData)==sizeof(SessionPersistDataOpaque)), "session persist data and the subclass should be the same size.");

#endif // defined(MBEDTLS_SSL_H)

}}

static_assert(sizeof(SessionPersistDataOpaque)==SessionPersistBaseSize+SessionPersistVariableSize, "SessionPersistDataOpque size should be SessionPersistBaseSize+SessionPersistVariableSize");

#endif // defined(__cplusplus)



