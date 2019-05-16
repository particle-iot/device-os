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

#include "logging.h"
LOG_SOURCE_CATEGORY("comm.dtls")

#include "dtls_message_channel.h"

#if HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL

#include "protocol.h"
#include "rng_hal.h"
#include "mbedtls/error.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls_util.h"
#include "mbedtls/version.h"
#include "timer_hal.h"
#include <stdio.h>
#include <string.h>
#include "dtls_session_persist.h"

namespace particle { namespace protocol {


uint32_t compute_checksum(uint32_t(*calculate_crc)(const uint8_t* data, uint32_t len), const uint8_t* server, size_t server_len, const uint8_t* device, size_t device_len)
{
	uint32_t sum[2];
	sum[0] = calculate_crc(server, server_len);
	sum[1] = calculate_crc(device, device_len);
	uint32_t result = calculate_crc((uint8_t*)&sum, sizeof(sum));
	return result;
}


void SessionPersist::prepare_save(const uint8_t* random, uint32_t keys_checksum, mbedtls_ssl_context* context, message_id_t next_id)
{
	if (context->state == MBEDTLS_SSL_HANDSHAKE_OVER)
	{
		this->keys_checksum = keys_checksum;
		in_epoch = context->in_epoch;
		memcpy(out_ctr, context->out_ctr, 8);
		memcpy(randbytes, random, sizeof(randbytes));
		this->next_coap_id = next_id;
		save_session(context->session);
		size = sizeof(*this);
	}
	else
	{
		size = 0;
	}
}

void SessionPersist::update(mbedtls_ssl_context* context, save_fn_t saver, message_id_t next_id)
{
	if (context->state == MBEDTLS_SSL_HANDSHAKE_OVER)
	{
		memcpy(out_ctr, context->out_ctr, 8);
		this->next_coap_id = next_id;
		save_this_with(saver);
	}
}

auto SessionPersist::restore(mbedtls_ssl_context* context, bool renegotiate, uint32_t keys_checksum, message_id_t* next_id, restore_fn_t restorer, save_fn_t saver) -> RestoreStatus
{
	if (!restore_this_from(restorer)) {
		return NO_SESSION;
	}

	if (!is_valid() || keys_checksum!=this->keys_checksum) {
		LOG(WARN,"discarding session: valid %d, keys_sum: %d/%d", is_valid(), keys_checksum, this->keys_checksum);
		return NO_SESSION;
	}

    LOG(WARN, "session has %d uses", use_count());
	if (has_expired()) {
	    invalidate();
	    save(saver);
	    LOG(WARN, "session has expired after %d uses", use_count());
	    return NO_SESSION;
	}

    increment_use_count();
    save(saver);

	// assume invalid initially. With the ssl context being reset,
	// we cannot return NO_SESSION from this point onwards.
	size = 0;

	mbedtls_ssl_session_reset(context);

	context->handshake->resume = 1;
	restore_session(context->session_negotiate);

	if (next_id)
		*next_id = this->next_coap_id;

	context->major_ver = MBEDTLS_SSL_MAJOR_VERSION_3;
	context->minor_ver = MBEDTLS_SSL_MINOR_VERSION_3;

	if (!renegotiate) {
		context->state = MBEDTLS_SSL_HANDSHAKE_WRAPUP;
		context->in_epoch = in_epoch;
		memcpy(context->out_ctr, &out_ctr, sizeof(out_ctr));
		memcpy(context->handshake->randbytes, randbytes, sizeof(randbytes));
		context->transform_negotiate->ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(ciphersuite);
		if (!context->transform_negotiate->ciphersuite_info)
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

		context->in_msg = context->in_iv + context->transform_negotiate->ivlen -
											context->transform_negotiate->fixed_ivlen;
		context->out_msg = context->out_iv + context->transform_negotiate->ivlen -
											 context->transform_negotiate->fixed_ivlen;

		context->session_in = context->session_negotiate;
		context->session_out = context->session_negotiate;

		context->transform_in = context->transform_negotiate;
		context->transform_out = context->transform_negotiate;

		mbedtls_ssl_handshake_wrapup(context);
		size = sizeof(*this);
		return COMPLETE;
	}
	else
	{
		size = sizeof(*this);
		return RENEGOTIATE;
	}
}

uint32_t SessionPersist::application_state_checksum(uint32_t (*calc_crc)(const uint8_t* data, uint32_t len))
{
	return Protocol::application_state_checksum(calc_crc, subscriptions_crc, describe_app_crc, describe_system_crc);
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
#if PLATFORM_ID!=3
	DEBUG_D("%s:%04d: %s", file, line, str);
#else
	fprintf(stdout, "%s:%04d: %s", file, line, str);
	fflush(stdout);
#endif
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

	mbedtls_ssl_conf_handshake_timeout(&conf, 3000, 6000);

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
	return NO_ERROR;
}

/*
 * Inspects the move session flag to amend the application data record to a move session record.
 * See: https://github.com/particle-iot/knowledge/blob/8df146d88c4237e90553f3fd6d8465ab58ec79e0/services/dtls-ip-change.md
 */
inline int DTLSMessageChannel::send(const uint8_t* data, size_t len)
{
	if (move_session && len && data[0]==23)
	{
		// buffer for a new packet that contains the device ID length and a byte for the length appended to the existing data.
		uint8_t d[len+DEVICE_ID_LEN+1];
		memcpy(d, data, len);						// original application data
		d[0] = 254;									// move session record type
		memcpy(d+len, device_id, DEVICE_ID_LEN);	// set the device ID
		d[len+DEVICE_ID_LEN] = DEVICE_ID_LEN;			// set the device ID length as the last byte in the packet
		int result = callbacks.send(d, len+DEVICE_ID_LEN+1, callbacks.tx_context);
		// hide the increased length from DTLS
		if (result==int(len+DEVICE_ID_LEN+1))
			result = len;
		return result;
	}
	else
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
	if (size>=0 && size <=1)
		size = 0;
	return size;
}

int DTLSMessageChannel::send_(void *ctx, const unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	int count = channel->send(buf, len);
	if (count == 0)
		return MBEDTLS_ERR_SSL_WANT_WRITE;

	return count;
}

int DTLSMessageChannel::recv_( void *ctx, unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;

	int count = channel->recv(buf, len);
	if (count == 0) {
		// 0 means no more data available yet
		return MBEDTLS_ERR_SSL_WANT_READ;
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
	mbedtls_debug_set_threshold(1);
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
	mbedtls_ssl_set_bio(&ssl_context, this, &DTLSMessageChannel::send_, &DTLSMessageChannel::recv_, NULL);

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

	return NO_ERROR;
}

ProtocolError DTLSMessageChannel::establish(uint32_t& flags, uint32_t app_state_crc)
{
	int ret = 0;
	// LOG(INFO,"setup context");
	ProtocolError error = setup_context();
	if (error) {
		LOG(ERROR,"setup_contex error %x", error);
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
		const uint32_t cached = sessionPersist.application_state_checksum(this->callbacks.calculate_crc);
		LOG(INFO,"app state crc: cached: %x, actual: %x", (unsigned)cached, (unsigned)app_state_crc);
		if (cached==app_state_crc) {
			LOG(WARN,"skipping hello message");
			flags |= Protocol::SKIP_SESSION_RESUME_HELLO;
		}
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

	if (ret)
	{
		LOG(ERROR,"handshake failed -%x", -ret);
		reset_session();
	}
	else
	{
		sessionPersist.prepare_save(random, keys_checksum, &ssl_context, 0);
	}
	return ret==0 ? NO_ERROR : IO_ERROR_GENERIC_ESTABLISH;
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
	if (ret<0) {
		switch (ret) {
		case MBEDTLS_ERR_SSL_WANT_READ:
			break;
		case MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE:
			ret = 0;
			break;
		case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
			command(CLOSE);
			break;
		default:
			reset_session();
			return IO_ERROR_GENERIC_RECEIVE;
		}
	}
	message.set_length(ret);
	if (ret>0) {
		cancel_move_session();
#if defined(DEBUG_BUILD) && 0
		if (LOG_ENABLED(TRACE)) {
		  LOG(TRACE, "msg len %d", message.length());
		  for (size_t i=0; i<message.length(); i++)
		  {
				  char buf[3];
				  char c = message.buf()[i];
				  sprintf(buf, "%02x", c);
				  LOG_PRINT(TRACE, buf);
		  }
		  LOG_PRINT(TRACE, "\r\n");
		}
#endif
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
  if (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER)
    return INVALID_STATE;

  if (message.send_direct())
  {
	  // send unencrypted
	  int bytes = this->send(message.buf(), message.length());
	  return bytes < 0 ? IO_ERROR_GENERIC_SEND : NO_ERROR;
  }

#if defined(DEBUG_BUILD) && 0
      LOG(TRACE, "msg len %d", message.length());
      for (size_t i=0; i<message.length(); i++)
      {
	  	  char buf[3];
	  	  char c = message.buf()[i];
	  	  sprintf(buf, "%02x", c);
	  	  LOG_PRINT(TRACE, buf);
      }
      LOG_PRINT(TRACE, "\r\n");
#endif

  int ret = mbedtls_ssl_write(&ssl_context, message.buf(), message.length());
  if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
  {
	  LOG(WARN, "mbedtls_ssl_write returned %x", ret);
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


}}

#include "sys/time.h"

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

#endif // HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL
