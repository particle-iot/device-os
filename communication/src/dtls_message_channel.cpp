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

#undef LOG_COMPILE_TIME_LEVEL

#include "logging.h"

LOG_SOURCE_CATEGORY("comm.dtls")

struct mbedtls_ssl_context;
struct mbedtls_ssl_transform;

extern "C" {

void mbedtls_ssl_update_in_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_update_out_pointers(mbedtls_ssl_context *ssl, mbedtls_ssl_transform *transform);

} // extern "C"

#include "dtls_message_channel.h"

#if HAL_PLATFORM_CLOUD_UDP

#include "protocol.h"
#include "rng_hal.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls_util.h"
#include "mbedtls/version.h"
#include "timer_hal.h"
#include <stdio.h>
#include <string.h>
#include "dtls_session_persist.h"
#include "coap_channel.h"
#include "coap_util.h"
#include "platforms.h"

namespace particle { namespace protocol {

namespace {

// A custom content type for session resumption packets
const unsigned ALT_CID_CONTENT_TYPE = 253;

} // namespace

uint32_t compute_checksum(uint32_t(*calculate_crc)(const uint8_t* data, uint32_t len), const uint8_t* server, size_t server_len, const uint8_t* device, size_t device_len)
{
	uint32_t sum[2];
	sum[0] = calculate_crc(server, server_len);
	sum[1] = calculate_crc(device, device_len);
	uint32_t result = calculate_crc((uint8_t*)&sum, sizeof(sum));
	return result;
}


bool SessionPersist::prepare_save(const uint8_t* random, uint32_t keys_checksum, mbedtls_ssl_context* context, message_id_t next_id)
{
	if (context->state != MBEDTLS_SSL_HANDSHAKE_OVER) {
		LOG(ERROR, "Invalid handshake state");
		return false;
	}

	uint8_t cid[MBEDTLS_SSL_CID_OUT_LEN_MAX] = {};
	size_t cidSize = 0;
	int cidEnabled = MBEDTLS_SSL_CID_DISABLED;
	int r = mbedtls_ssl_get_peer_cid(context, &cidEnabled, cid, &cidSize);
	if (r != 0 || cidEnabled != MBEDTLS_SSL_CID_ENABLED || cidSize != DTLS_CID_SIZE) {
		LOG(ERROR, "Unable to get connection ID");
		return false;
	}
	memcpy(this->cid, cid, DTLS_CID_SIZE);

	this->keys_checksum = keys_checksum;
	in_epoch = context->in_epoch;
	memcpy(out_ctr, context->cur_out_ctr, 8);
	memcpy(randbytes, random, sizeof(randbytes));
	this->next_coap_id = next_id;
	save_session(context->session);

	size = sizeof(*this);

	return true;
}

void SessionPersist::update(mbedtls_ssl_context* context, save_fn_t saver, message_id_t next_id)
{
	if (context->state == MBEDTLS_SSL_HANDSHAKE_OVER)
	{
		memcpy(out_ctr, context->cur_out_ctr, 8);
		in_epoch = context->in_epoch;
		in_window_top = context->in_window_top;
		in_window = context->in_window;
		this->next_coap_id = next_id;
		save_this_with(saver);
	}
}

SessionPersist::RestoreStatus SessionPersist::restore(mbedtls_ssl_context* context, bool renegotiate,
		uint32_t keys_checksum, message_id_t* next_id, restore_fn_t restorer, save_fn_t saver)
{
	if (!restore_this_from(restorer)) {
		return NO_SESSION;
	}

	if (!is_valid() || keys_checksum!=this->keys_checksum) {
		LOG(WARN, "discarding session: valid %d, keys_sum: %d/%d", is_valid(), keys_checksum, this->keys_checksum);
		return NO_SESSION;
	}

	if (has_expired()) {
		LOG(WARN, "session has expired after %d uses", use_count());
		invalidate();
		save(saver);
		return NO_SESSION;
	} else {
		LOG(INFO, "session has %d uses", use_count());
	}

	increment_use_count();
	save(saver);

	// assume invalid initially. With the ssl context being reset,
	// we cannot return NO_SESSION from this point onwards.
	size = 0;

	mbedtls_ssl_session_reset(context);

	context->handshake->resume = 1;
	restore_session(context->session_negotiate);

	if (next_id) {
		*next_id = this->next_coap_id;
	}

	context->major_ver = MBEDTLS_SSL_MAJOR_VERSION_3;
	context->minor_ver = MBEDTLS_SSL_MINOR_VERSION_3;

	if (!renegotiate) {
		context->state = MBEDTLS_SSL_HANDSHAKE_WRAPUP;
		context->in_epoch = in_epoch;
		context->in_window_top = in_window_top;
		context->in_window = in_window;
		memcpy(context->cur_out_ctr, &out_ctr, sizeof(out_ctr));
		memcpy(context->handshake->randbytes, randbytes, sizeof(randbytes));
		const auto ciphersuiteInfo = mbedtls_ssl_ciphersuite_from_id(ciphersuite);
		if (!ciphersuiteInfo)
		{
			LOG(ERROR,"unknown ciphersuite with id %d", ciphersuite);
			return ERROR;
		}

		int err = mbedtls_ssl_derive_keys(context);
		if (err)
		{
			LOG(ERROR,"derive keys failed with %d", err);
			return ERROR;
		}

		context->transform_negotiate->out_cid_len = DTLS_CID_SIZE;
		memcpy(context->transform_negotiate->out_cid, cid, DTLS_CID_SIZE);
		mbedtls_ssl_update_out_pointers(context, context->transform_negotiate);
		mbedtls_ssl_update_in_pointers(context);

		context->session_in = context->session_negotiate;
		context->session_out = context->session_negotiate;

		context->transform_in = context->transform_negotiate;
		context->transform_out = context->transform_negotiate;

		mbedtls_ssl_handshake_wrapup(context);
		size = sizeof(*this);
		return COMPLETE;
	} else {
		size = sizeof(*this);
		return RENEGOTIATE;
	}
}

AppStateDescriptor SessionPersist::app_state_descriptor()
{
	if (!is_valid()) {
		return AppStateDescriptor();
	}
	return AppStateDescriptor(app_state_flags, system_version, describe_system_crc, describe_app_crc, subscriptions_crc,
			protocol_flags, max_message_size, max_binary_size, ota_chunk_size);
}

SessionPersist sessionPersist;

// mbedtls_ecp_gen_keypair
// see also gen_key.c and mbedtls_ecp_gen_key

#define EXIT_ERROR(x, msg) \
	if (x) { \
		LOG(WARN,"DTLS init failure: " #msg ": %c%04X",(x<0)?'-':' ',(x<0)?-x:x);\
		return UNKNOWN; \
	}

static void my_debug(void *ctx, int level,
		const char *file, int line,
		const char *str )
{
#if PLATFORM_ID != PLATFORM_GCC
	DEBUG_D("%s:%04d: %s", file, line, str);
#else
	fprintf(stdout, "%s:%04d: %s", file, line, str);
	fflush(stdout);
#endif // PLATFORM_ID != PLATFORM_GCC
}

ProtocolError DTLSMessageChannel::init(
		const uint8_t* core_private, size_t core_private_len,
		const uint8_t* core_public, size_t core_public_len,
		const uint8_t* server_public, size_t server_public_len,
		const uint8_t* device_id, Callbacks& callbacks,
		message_id_t* coap_state)
{
	init();
	this->coap_state = coap_state;
	int ret;
	this->callbacks = callbacks;
	this->device_id = device_id;
	keys_checksum = compute_checksum(callbacks.calculate_crc, server_public, server_public_len, core_private, core_private_len);

	ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
	EXIT_ERROR(ret, "unable to configure defaults");

	mbedtls_ssl_conf_handshake_timeout(&conf, 3000, 24000);

	mbedtls_ssl_conf_rng(&conf, mbedtls_default_rng, nullptr); // todo - would like to make this a callback
	mbedtls_ssl_conf_dbg(&conf, my_debug, nullptr);
	mbedtls_ssl_conf_min_version(&conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

	ret = mbedtls_pk_parse_public_key(&pkey, core_public, core_public_len);
	EXIT_ERROR(ret, "unable to parse device pub key");

	ret = mbedtls_pk_parse_key(&pkey, core_private, core_private_len, NULL, 0);
	EXIT_ERROR(ret, "unable to parse device private key");

	ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey);
	EXIT_ERROR(ret, "unable to config own cert");

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

	static int ssl_cert_types[] = { MBEDTLS_TLS_CERT_TYPE_RAW_PUBLIC_KEY, MBEDTLS_TLS_CERT_TYPE_NONE };
	mbedtls_ssl_conf_client_certificate_types(&conf, ssl_cert_types);
	mbedtls_ssl_conf_server_certificate_types(&conf, ssl_cert_types);
	mbedtls_ssl_conf_certificate_receive(&conf, MBEDTLS_SSL_RECEIVE_CERTIFICATE_DISABLED);

	this->server_public = new uint8_t[server_public_len];
	memcpy(this->server_public, server_public, server_public_len);
	this->server_public_len = server_public_len;

	// Records received from the server are not expected to contain a CID
	mbedtls_ssl_conf_cid(&conf, 0 /* len */, MBEDTLS_SSL_UNEXPECTED_CID_IGNORE);

	return NO_ERROR;
}

inline int DTLSMessageChannel::send(const uint8_t* data, size_t len)
{
	if (move_session && len > 0 && data[0] == MBEDTLS_SSL_MSG_CID) {
		const auto d = const_cast<uint8_t*>(data);
		d[0] = ALT_CID_CONTENT_TYPE;
		const auto r = callbacks.send(d, len, callbacks.tx_context);
		d[0] = MBEDTLS_SSL_MSG_CID;
		return r;
	}
	return callbacks.send(data, len, callbacks.tx_context);
}

void DTLSMessageChannel::reset_session()
{
	cancel_move_session();
	mbedtls_ssl_session_reset(&ssl_context);
	sessionPersist.clear(callbacks.save);
}

inline int DTLSMessageChannel::recv(uint8_t* data, size_t len)
{
	int size = callbacks.receive(data, len, callbacks.tx_context);
	// ignore 0 and 1 byte UDP packets which are used to keep alive the connection.
	if (size == 1) {
		size = 0;
	}
	return size;
}

int DTLSMessageChannel::sendCallback(void *ctx, const unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	int count = channel->send(buf, len);
	if (count == 0) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	} else if (count < 0) {
		return MBEDTLS_ERR_NET_SEND_FAILED;
	}
	return count;
}

int DTLSMessageChannel::recvCallback( void *ctx, unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	int count = channel->recv(buf, len);
	if (count == 0) {
		// 0 means no more data available yet
		return MBEDTLS_ERR_SSL_WANT_READ;
	} else if (count < 0) {
		return MBEDTLS_ERR_NET_RECV_FAILED;
	}
	return count;
}

void DTLSMessageChannel::init()
{
	server_public = nullptr;
	mbedtls_ssl_init (&ssl_context);
	mbedtls_ssl_config_init (&conf);
	mbedtls_x509_crt_init (&clicert);
	mbedtls_pk_init (&pkey);

#if defined(MBEDTLS_DEBUG_C)
#ifndef MBEDTLS_DEBUG_COMPILE_TIME_LEVEL
	mbedtls_debug_set_threshold(1);
#else
	mbedtls_debug_set_threshold(MBEDTLS_DEBUG_COMPILE_TIME_LEVEL);
#endif // MBEDTLS_DEBUG_COMPILE_TIME_LEVEL
#endif // MBEDTLS_DEBUG_C
}

void DTLSMessageChannel::dispose()
{
	mbedtls_x509_crt_free (&clicert);
	mbedtls_pk_free (&pkey);
	mbedtls_ssl_config_free (&conf);
	mbedtls_ssl_free (&ssl_context);
	delete this->server_public;
	server_public_len = 0;
}


ProtocolError DTLSMessageChannel::setup_context()
{
	int ret;
	mbedtls_ssl_free(&ssl_context);
	ret = mbedtls_ssl_setup(&ssl_context, &conf);
	EXIT_ERROR(ret, "unable to setup SSL context");

	mbedtls_ssl_set_timer_cb(&ssl_context, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
	mbedtls_ssl_set_bio(&ssl_context, this, &DTLSMessageChannel::sendCallback, &DTLSMessageChannel::recvCallback, NULL);

	if ((ssl_context.session_negotiate->peer_cert = (mbedtls_x509_crt*)calloc(1, sizeof(mbedtls_x509_crt))) == NULL)
	{
		LOG(ERROR,"unable to allocate cert storage");
		return INSUFFICIENT_STORAGE;
	}

	mbedtls_x509_crt_init(ssl_context.session_negotiate->peer_cert);
	ret = mbedtls_pk_parse_public_key(&ssl_context.session_negotiate->peer_cert->pk, server_public, server_public_len);
	if (ret) {
		LOG(WARN,"unable to parse negotiated pub key: -%x", -ret);
		return IO_ERROR_PARSING_SERVER_PUBLIC_KEY;
	}

	// Enable the extension but do not set a CID
	ret = mbedtls_ssl_set_cid(&ssl_context, MBEDTLS_SSL_CID_ENABLED, nullptr /* own_cid */, 0 /* own_cid_len */);
	if (ret != 0) {
		LOG(ERROR, "mbedtls_ssl_set_cid() failed: -0x%x", -ret);
		return ProtocolError::INTERNAL;
	}

	return NO_ERROR;
}

ProtocolError DTLSMessageChannel::establish()
{
	int ret = 0;
	// LOG(INFO,"setup context");
	ProtocolError error = setup_context();
	if (error) {
		LOG(ERROR,"setup_context error %d", (int)error);
		return error;
	}
	bool renegotiate = false;

	SessionPersist::RestoreStatus restoreStatus = sessionPersist.restore(&ssl_context, renegotiate, keys_checksum, coap_state, callbacks.restore, callbacks.save);
	LOG(INFO,"(CMPL,RENEG,NO_SESS,ERR) restoreStatus=%d", restoreStatus);
	if (restoreStatus==SessionPersist::COMPLETE)
	{
		LOG(INFO,"out_ctr %d,%d,%d,%d,%d,%d,%d,%d, next_coap_id=%x", sessionPersist.out_ctr[0],
				sessionPersist.out_ctr[1],sessionPersist.out_ctr[2],sessionPersist.out_ctr[3],
				sessionPersist.out_ctr[4],sessionPersist.out_ctr[5],sessionPersist.out_ctr[6],
				sessionPersist.out_ctr[7], sessionPersist.next_coap_id);
		sessionPersist.make_persistent();
		LOG(INFO,"restored session from persisted session data. next_msg_id=%d", *coap_state);
		return SESSION_RESUMED;
	}
	else if (restoreStatus==SessionPersist::RENEGOTIATE)
	{
		// session partially restored, fully restored via handshake
	}
	else // no session or clear
	{
		reset_session();
		ProtocolError error = setup_context();
		if (error)
			return error;
	}
	uint8_t random[64];

	do
	{
		while (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER)
		{
			ret = mbedtls_ssl_handshake_step(&ssl_context);

			if (ret != 0)
				break;

			// we've already received the ServerHello, thus
			// we have the random values for client and server
			if (ssl_context.state == MBEDTLS_SSL_SERVER_KEY_EXCHANGE)
			{
				memcpy(random, ssl_context.handshake->randbytes, 64);
			}
		}
	}
	while(ret == MBEDTLS_ERR_SSL_WANT_READ ||
	      ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	bool ok = false;
	if (ret) {
		LOG(ERROR, "handshake failed -%x", -ret);
	} if (sessionPersist.prepare_save(random, keys_checksum, &ssl_context, 0)) {
		ok = true;
	}
	if (!ok) {
		reset_session();
		return IO_ERROR_GENERIC_ESTABLISH;
	}

	return NO_ERROR;
}

ProtocolError DTLSMessageChannel::notify_established()
{
	sessionPersist.make_persistent();
	sessionPersist.save(callbacks.save);
	return NO_ERROR;
}

ProtocolError DTLSMessageChannel::receive(Message& message)
{
	if (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER)
		return INVALID_STATE;

	create(message);
	uint8_t* buf = message.buf();
	size_t len = message.capacity();

	conf.read_timeout = 0;
	int ret = mbedtls_ssl_read(&ssl_context, buf, len);
	if (ret < 0) {
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE) {
			ret = 0;
		} else {
			LOG(ERROR, "mbedtls_ssl_read() failed: -0x%x", -ret);
			switch (ret) {
			// mbedtls_ssl_read() may need to flush the output before attempting to read from the socket
			// so we need to handle both MBEDTLS_ERR_NET_SEND_FAILED and MBEDTLS_ERR_NET_RECV_FAILED here
			case MBEDTLS_ERR_NET_SEND_FAILED:
			case MBEDTLS_ERR_NET_RECV_FAILED:
				// Do not invalidate the session on network errors
				return IO_ERROR_SOCKET_RECV_FAILED;
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				command(CLOSE);
				return IO_ERROR_REMOTE_END_CLOSED;
			default:
				reset_session();
				return IO_ERROR_GENERIC_RECEIVE;
			}
		}
	}
	message.set_length(ret);
	if (ret > 0) {
		cancel_move_session();
		sessionPersist.update(&ssl_context, callbacks.save, coap_state ? *coap_state : 0);
		if (debug_enabled) {
			LOG_C(TRACE, COAP_LOG_CATEGORY, "Received CoAP message");
			logCoapMessage(LOG_LEVEL_TRACE, COAP_LOG_CATEGORY, (const char*)message.buf(), message.length());
		}
	}
	return NO_ERROR;
}

/**
 * Once data has been successfully received we can stop
 * sending move-session messages.
 * This is also used to reset the expiration counter.
 */
void DTLSMessageChannel::cancel_move_session()
{
    if (move_session) {
        move_session = false;
        sessionPersist.clear_use_count();
        command(SAVE_SESSION);
    }
}

ProtocolError DTLSMessageChannel::send(Message& message)
{
	if (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER) {
		return INVALID_STATE;
	}

	if (message.send_direct()) {
		// send unencrypted
		int bytes = this->send(message.buf(), message.length());
		return bytes < 0 ? IO_ERROR_GENERIC_SEND : NO_ERROR;
	}

	if (debug_enabled) {
		LOG_C(TRACE, COAP_LOG_CATEGORY, "Sending CoAP message");
		logCoapMessage(LOG_LEVEL_TRACE, COAP_LOG_CATEGORY, (const char*)message.buf(), message.length());
	}

	int ret = mbedtls_ssl_write(&ssl_context, message.buf(), message.length());
	if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
		LOG(ERROR, "mbedtls_ssl_write() failed: -0x%x", -ret);
		if (ret == MBEDTLS_ERR_NET_SEND_FAILED) {
			// Do not invalidate the session on network errors
			return IO_ERROR_SOCKET_SEND_FAILED;
		}
		reset_session();
		return IO_ERROR_GENERIC_MBEDTLS_SSL_WRITE;
	}
	sessionPersist.update(&ssl_context, callbacks.save, coap_state ? *coap_state : 0);
	return NO_ERROR;
}

bool DTLSMessageChannel::is_unreliable()
{
	return true;
}

ProtocolError DTLSMessageChannel::command(Command command, void* arg)
{
	LOG(INFO,"session cmd (CLS,DIS,MOV,LOD,SAV): %d", command);
	switch (command)
	{
	case CLOSE:
		reset_session();
		break;

	case DISCARD_SESSION:
		reset_session();
		return IO_ERROR_DISCARD_SESSION; //force re-establish

	case MOVE_SESSION:
		move_session = true;
		break;

	case LOAD_SESSION:
		sessionPersist.restore(callbacks.restore);
		break;

	case SAVE_SESSION:
		sessionPersist.save(callbacks.save);
		break;
	}
	return NO_ERROR;
}

AppStateDescriptor DTLSMessageChannel::cached_app_state_descriptor() const
{
	return sessionPersist.app_state_descriptor();
}


}}

// FIXME:
#if !HAL_PLATFORM_FILESYSTEM
#include <sys/time.h>

extern "C" int _gettimeofday( struct timeval *tv, void *tzvp )
{
    mbedtls_callbacks_t* cb = mbedtls_get_callbacks(NULL);
    uint32_t t = 0;
    if (cb && cb->millis) {
        t = cb->millis();
    } else {
        return -1;
    }
    tv->tv_sec = t / 1000;  // convert to seconds
    tv->tv_usec = ( t % 1000 )*1000;  // get remaining microseconds
    return 0;  // return non-zero for error
} // end _gettimeofday()
#endif // HAL_PLATFORM_FILESYSTEM

#endif // HAL_PLATFORM_CLOUD_UDP
