
#include "wiced.h"
#include "wiced_security.h"
#include "crypto_open/x509.h"
#include "crypto_open/bignum.h"
#include "micro-ecc/configuration.h"
#include "micro-ecc/uECC.h"

// <stubbed>
void sha4_starts(sha4_context* context, int32_t is384) {

}
void sha4_update(sha4_context* context, const unsigned char* input_data, int32_t input_len) {

}
void sha4_finish(sha4_context* context, unsigned char hash_output[64]) {

}
int uECC_sign(const uint8_t private_key[uECC_BYTES], const uint8_t message_hash[uECC_BYTES], uint8_t signature[uECC_BYTES*2]) {
    return 0;
}

int uECC_verify(const uint8_t public_key[uECC_BYTES*2], const uint8_t hash[uECC_BYTES], const uint8_t signature[uECC_BYTES*2]) {
    return 0;
}

int32_t x509parse_key_ecc(wiced_tls_ecc_key_t * ecc, const unsigned char *key, uint32_t keylen, const unsigned char *pwd, uint32_t pwdlen) {
    return 1;
}

#if 0
wiced_result_t wiced_tcp_start_tls( wiced_tcp_socket_t* socket, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_close_notify( wiced_tcp_socket_t* socket )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_deinit_context( wiced_tls_context_t* tls_context )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_calculate_overhead( wiced_tls_context_t* context, uint16_t content_length, uint16_t* header, uint16_t* footer )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_encrypt_packet( wiced_tls_context_t* context, wiced_packet_t* packet )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_receive_packet( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_reset_context( wiced_tls_context_t* tls_context )
{
    return WICED_SUCCESS;
}

void host_network_process_eapol_data( /*@only@*/ wiced_buffer_t buffer, wwd_interface_t interface )
{
}

wiced_result_t wiced_tls_init_context( wiced_tls_context_t* context, wiced_tls_identity_t* identity, const char* peer_cn )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_enable_tls( wiced_tcp_socket_t* socket, void* context )
{
    return WICED_SUCCESS;
}

void ssl_free(ssl_context *ssl)
{
}

wiced_result_t wiced_dtls_calculate_overhead( wiced_dtls_workspace_t* context, uint16_t available_space, uint16_t* header, uint16_t* footer )
{
	return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_close_notify( wiced_udp_socket_t* socket )
{
	return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_deinit_context( wiced_dtls_context_t* context )
{
	return WICED_SUCCESS;
}

wiced_bool_t wiced_tls_is_encryption_enabled( wiced_tcp_socket_t* socket )
{
	return WICED_FALSE;
}
#endif