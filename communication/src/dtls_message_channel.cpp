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

namespace particle { namespace protocol {

// mbedtls_ecp_gen_keypair
// see also gen_key.c and mbedtls_ecp_gen_key

#define EXIT_ERROR(ret) \
	if (ret) return UNKNOWN

static void my_debug( void *ctx, int level,
		const char *file, int line,
		const char *str )
{

}

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

	int ret = mbedtls_pk_parse_public_key(&pkey, core_public, core_public_len);
	EXIT_ERROR(ret);

	ret = mbedtls_pk_parse_key(&pkey, core_private, core_private_len, NULL, 0);
	EXIT_ERROR(ret);

	ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
	EXIT_ERROR(ret);

	mbedtls_ssl_conf_min_version(&conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

	mbedtls_ssl_conf_rng(&conf, dtls_rng, nullptr);
	mbedtls_ssl_conf_dbg(&conf, my_debug, nullptr);

	ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey);
	EXIT_ERROR(ret);

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	static int ssl_cert_types[] = { MBEDTLS_TLS_CERT_TYPE_RAW_PUBLIC_KEY, MBEDTLS_TLS_CERT_TYPE_NONE };
	mbedtls_ssl_conf_client_certificate_types(&conf, ssl_cert_types);
	mbedtls_ssl_conf_server_certificate_types(&conf, ssl_cert_types);
	mbedtls_ssl_conf_certificate_receive(&conf, MBEDTLS_SSL_RECEIVE_CERTIFICATE_DISABLED);

	ret = mbedtls_ssl_setup(&ssl_context, &conf);
	EXIT_ERROR(ret);

	if ((ssl_context.session_negotiate->peer_cert = (mbedtls_x509_crt*)calloc(1, sizeof(mbedtls_x509_crt))) == NULL)
	{
		return INSUFFICIENT_STORAGE;
	}

	mbedtls_x509_crt_init(ssl_context.session_negotiate->peer_cert);
	ret = mbedtls_pk_parse_public_key(&ssl_context.session_negotiate->peer_cert->pk, server_public, server_public_len);
	EXIT_ERROR(ret);

	mbedtls_ssl_set_timer_cb(&ssl_context, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
	mbedtls_ssl_set_bio(&ssl_context, this, DTLSMessageChannel::send, DTLSMessageChannel::recv, NULL);

	return NO_ERROR;
}

int DTLSMessageChannel::send( void *ctx, const unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	return channel->send(buf, len);
}

int DTLSMessageChannel::recv( void *ctx, unsigned char *buf, size_t len ) {
	DTLSMessageChannel* channel = (DTLSMessageChannel*)ctx;
	return channel->recv(buf, len);
}


void DTLSMessageChannel::init()
{
	mbedtls_ssl_init (&ssl_context);
	mbedtls_ssl_config_init (&conf);
	mbedtls_x509_crt_init (&clicert);
	mbedtls_pk_init (&pkey);
	mbedtls_entropy_init (&entropy);
	mbedtls_ctr_drbg_init (&ctr_drbg);

//	mbedtls_debug_set_threshold(debug_level);
}

void DTLSMessageChannel::dispose()
{
	mbedtls_x509_crt_free (&clicert);
	mbedtls_pk_free (&pkey);
	mbedtls_ssl_config_free (&conf);
	mbedtls_ctr_drbg_free (&ctr_drbg);
	mbedtls_entropy_free (&entropy);
	mbedtls_ssl_free (&ssl_context);
}


ProtocolError DTLSMessageChannel::establish()
{
    return NO_ERROR;
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
    if (message.length()>20)
        DEBUG("message length %d, last 20 bytes %s ", message.length(), message.buf()+message.length()-20);
    else
        DEBUG("message length %d ", message.length());

    return mbedtls_ssl_write(&ssl_context, message.buf(), message.length())<0 ? IO_ERROR : NO_ERROR;
}


}}
