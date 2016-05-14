
#include "eckeygen.h"
#include "protocol_selector.h"
#include "hal_platform.h"

#if PARTICLE_PROTOCOL && HAL_PLATFORM_CLOUD_UDP
#include "mbedtls/pk.h"
#include "mbedtls/asn1.h"
#include <string.h>

int gen_ec_key(uint8_t* buffer, size_t max_length, int (*f_rng) (void *, uint8_t* buf, size_t len), void *p_rng)
{
	mbedtls_pk_context key;
	memset(&key, 0, sizeof(key));
	int error = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
	if (!error)
		error = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), f_rng, p_rng );
	if (!error) {
		int result = mbedtls_pk_write_key_der(&key, buffer, max_length);
		if (result<0)
			error = result;
		else if (result>0)
		{
			// the key is written to the end of the buffer - align to the start
			memmove(buffer, buffer+max_length-result, result);
		}
	}
	mbedtls_pk_free(&key);
	return error;
}

size_t determine_der_length(const uint8_t* key, size_t max_len)
{
	size_t len;
	const uint8_t* end = key+max_len;
	const uint8_t* p = key;
	if (mbedtls_asn1_get_tag( (uint8_t**)&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
		return 0;
	}
	return p-key+len;
}

int extract_public_ec_key_length(uint8_t* buffer, size_t max_length, const uint8_t* private_key, size_t private_key_len)
{
	mbedtls_pk_context key;
	mbedtls_pk_init(&key);
	int error = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
	if (!error)
		error = mbedtls_pk_parse_key(&key, private_key, private_key_len, nullptr, 0);
	if (!error)
		error = mbedtls_pk_write_pubkey_der(&key, buffer, max_length);

	mbedtls_pk_free(&key);
	return error;
}

int extract_public_ec_key(uint8_t* buffer, size_t max_length, const uint8_t* private_key)
{
	return extract_public_ec_key_length(buffer, max_length, private_key, determine_der_length(private_key, max_length));
}


#endif
