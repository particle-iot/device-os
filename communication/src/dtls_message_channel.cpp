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

#include "dtls_message_channel.h"
#include "service_debug.h"
#include "rng_hal.h"
#include "mbedtls/error.h"
#include "timer_hal.h"
#include <stdio.h>

namespace particle { namespace protocol {

// mbedtls_ecp_gen_keypair
// see also gen_key.c and mbedtls_ecp_gen_key

#define EXIT_ERROR(x, msg) \
	if (x) { \
		WARN("DTLS initialization failure: " #msg ": %c%04X",(x<0)?'-':' ',(x<0)?-x:x);\
		return UNKNOWN; \
	}

static void my_debug( void *ctx, int level,
		const char *file, int line,
		const char *str )
{
	DEBUG("%s", str);
}

// todo - would like to make this a callback
static int dtls_rng(void* handle, uint8_t* data, size_t len)
{
	while (len>=4)
	{
		*((uint32_t*)data) = HAL_RNG_GetRandomNumber();
		data += 4;
		len -= 4;
	}
	while (len-->0)
	{
		*data++ = HAL_RNG_GetRandomNumber();
	}
	return 0;
}


ProtocolError DTLSMessageChannel::init(
		const uint8_t* core_private, size_t core_private_len,
		const uint8_t* core_public, size_t core_public_len,
		const uint8_t* server_public, size_t server_public_len,
		const uint8_t* device_id, Callbacks& callbacks)
{
	init();
	int ret;

	this->callbacks = callbacks;

	mbedtls_debug_set_threshold(255);

	ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
	EXIT_ERROR(ret, "unable to configure defaults");

	mbedtls_ssl_conf_rng(&conf, dtls_rng, nullptr);
	mbedtls_ssl_conf_dbg(&conf, my_debug, nullptr);
	mbedtls_ssl_conf_min_version(&conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

	ret = mbedtls_pk_parse_public_key(&pkey, core_public, core_public_len);
	EXIT_ERROR(ret, "unable to parse device public key");

	ret = mbedtls_pk_parse_key(&pkey, core_private, core_private_len, NULL, 0);
	EXIT_ERROR(ret, "unable to parse device private key");

	ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey);
	EXIT_ERROR(ret, "unable to configure own certificate");

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	static int ssl_cert_types[] = { MBEDTLS_TLS_CERT_TYPE_RAW_PUBLIC_KEY, MBEDTLS_TLS_CERT_TYPE_NONE };
	mbedtls_ssl_conf_client_certificate_types(&conf, ssl_cert_types);
	mbedtls_ssl_conf_server_certificate_types(&conf, ssl_cert_types);
	mbedtls_ssl_conf_certificate_receive(&conf, MBEDTLS_SSL_RECEIVE_CERTIFICATE_DISABLED);

	ret = mbedtls_ssl_setup(&ssl_context, &conf);
	EXIT_ERROR(ret, "unable to setup SSL context");

	if ((ssl_context.session_negotiate->peer_cert = (mbedtls_x509_crt*)calloc(1, sizeof(mbedtls_x509_crt))) == NULL)
	{
		ERROR("unable to allocate certificate storage");
		return INSUFFICIENT_STORAGE;
	}

	mbedtls_x509_crt_init(ssl_context.session_negotiate->peer_cert);
	ret = mbedtls_pk_parse_public_key(&ssl_context.session_negotiate->peer_cert->pk, server_public, server_public_len);
	EXIT_ERROR(ret, "unable to parse netogiated public key");

	mbedtls_ssl_set_timer_cb(&ssl_context, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
	mbedtls_ssl_set_bio(&ssl_context, this, &DTLSMessageChannel::send_, &DTLSMessageChannel::recv_, NULL);

	return NO_ERROR;
}

inline int DTLSMessageChannel::send(const uint8_t* data, size_t len)
{
	return callbacks.send(data, len, callbacks.tx_context);
}

inline int DTLSMessageChannel::recv(uint8_t* data, size_t len)
{
	return callbacks.receive(data, len, callbacks.tx_context);
}


int DTLSMessageChannel::send_( void *ctx, const unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	return channel->send(buf, len);
}

int DTLSMessageChannel::recv_( void *ctx, unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	int count = channel->recv(buf, len);
	if (count == 0)
	{
		// 0 means EOF in this context
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	return count;
}


void DTLSMessageChannel::init()
{
	mbedtls_ssl_init (&ssl_context);
	mbedtls_ssl_config_init (&conf);
	mbedtls_x509_crt_init (&clicert);
	mbedtls_pk_init (&pkey);
//	mbedtls_debug_set_threshold(debug_level);
}

void DTLSMessageChannel::dispose()
{
	mbedtls_x509_crt_free (&clicert);
	mbedtls_pk_free (&pkey);
	mbedtls_ssl_config_free (&conf);
	mbedtls_ssl_free (&ssl_context);
}

ProtocolError DTLSMessageChannel::establish()
{
	int ret = mbedtls_ssl_handshake(&ssl_context);
	if (ret)
	{
		DEBUG("handshake failed %x", ret);
	}
	return ret==0 ? NO_ERROR : IO_ERROR;
}

ProtocolError DTLSMessageChannel::receive(Message& message)
{
	if (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER)
		return INVALID_STATE;

	create(message);
	uint8_t* buf = message.buf();
	size_t len = message.capacity();

	memset(buf, 0, len);
	conf.read_timeout = 0;
	int ret = mbedtls_ssl_read(&ssl_context, buf, len);
	if (ret <= 0) {
		return IO_ERROR;
	}
	message.set_length(ret);
	return NO_ERROR;
}

ProtocolError DTLSMessageChannel::send(Message& message)
{
	if (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER)
		return INVALID_STATE;

    if (message.length()>20)
        DEBUG("message length %d, last 20 bytes %s ", message.length(), message.buf()+message.length()-20);
    else
        DEBUG("message length %d ", message.length());

    return mbedtls_ssl_write(&ssl_context, message.buf(), message.length())<0 ? IO_ERROR : NO_ERROR;
}


}}

#include "sys/time.h"

extern "C" unsigned long mbedtls_timing_hardclock()
{
	return HAL_Timer_Microseconds();
}

extern "C" int _gettimeofday( struct timeval *tv, void *tzvp )
{
    uint32_t t = HAL_Timer_Microseconds();  // get uptime in nanoseconds
    tv->tv_sec = t / 1000000;  // convert to seconds
    tv->tv_usec = ( t % 1000000 );  // get remaining microseconds
    return 0;  // return non-zero for error
} // end _gettimeofday()


