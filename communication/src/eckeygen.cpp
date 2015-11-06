
#include "eckeygen.h"
#include "protocol_selector.h"
#include "hal_platform.h"

#ifdef PARTICLE_PROTOCOL
#if HAL_PLATFORM_CLOUD_UDP
#include "mbedtls/pk.h"

int gen_ec_key(uint8_t* buffer, size_t max_length, int (*f_rng) (void *, uint8_t* buf, size_t len), void *p_rng)
{
	mbedtls_pk_context key;
	int error = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
	if (!error)
		error = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256K1, mbedtls_pk_ec(key), f_rng, p_rng );
	if (!error) {
		int result = mbedtls_pk_write_key_der(&key, buffer, max_length);
		if (result<0)
			error = result;
	}
	mbedtls_pk_free(&key);
	return error;
}

int extract_public_ec_key(uint8_t* buffer, size_t max_length, const uint8_t* private_key)
{
	mbedtls_pk_context key;
	int error = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
	if (!error)
		error = mbedtls_pk_parse_key(&key, private_key, max_length, nullptr, 0);
	if (!error)
		error = mbedtls_pk_write_pubkey_der(&key, buffer, max_length);

	mbedtls_pk_free(&key);
	return error;
}


#endif
#endif
