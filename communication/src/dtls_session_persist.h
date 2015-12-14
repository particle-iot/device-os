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

#ifdef __cplusplus

#include "dtls_message_channel.h"

struct __attribute__((packed)) SessionPersistOpaque
{
	uint16_t size;
	uint8_t data[156-2+sizeof(mbedtls_ssl_session::ciphersuite)+sizeof(mbedtls_ssl_session::id_len)+sizeof(mbedtls_ssl_session::compression)];
};

namespace particle { namespace protocol {


class __attribute__((packed)) SessionPersist
{
public:

	using save_fn_t = decltype(DTLSMessageChannel::Callbacks::save);
	using restore_fn_t = decltype(DTLSMessageChannel::Callbacks::restore);

private:

	uint16_t size;

	uint8_t randbytes[sizeof(mbedtls_ssl_handshake_params::randbytes)];
	decltype(mbedtls_ssl_session::ciphersuite) ciphersuite;
	decltype(mbedtls_ssl_session::compression) compression;
	decltype(mbedtls_ssl_session::id_len) id_len;
	uint8_t id[sizeof(mbedtls_ssl_session::id)];
	uint8_t master[sizeof(mbedtls_ssl_session::master)];
	decltype(mbedtls_ssl_context::in_epoch) in_epoch;
	unsigned char out_ctr[8];

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

	bool restore_this_from(restore_fn_t restorer)
	{
		if (restorer)
		{
			if (restorer(this, sizeof(*this))!=sizeof(*this))
				return false;
		}
		return true;
	}

	void save_this_with(save_fn_t saver)
	{
		if (saver)
			saver(this, sizeof(*this));
	}

public:

	void clear(save_fn_t saver)
	{
		size = 0;
		save_this_with(saver);
	}

	void save(const uint8_t* random, mbedtls_ssl_context* context, save_fn_t saver);
	void update(mbedtls_ssl_context* context, save_fn_t saver);

	bool restore(mbedtls_ssl_context* context, bool renegotiate, restore_fn_t restorer);

};

static_assert(sizeof(SessionPersist)==156+sizeof(mbedtls_ssl_session::ciphersuite)+sizeof(mbedtls_ssl_session::id_len)+sizeof(mbedtls_ssl_session::compression), "SessionPersist size");
static_assert(sizeof(SessionPersist)==sizeof(SessionPersistOpaque), "SessionPersistOpaque size == sizeof(SessionPersistQueue)");

#endif

}}


