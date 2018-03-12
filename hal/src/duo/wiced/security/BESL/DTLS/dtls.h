/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef _DTLS_DTLS_H_
#define _DTLS_DTLS_H_

#include "dtls_types.h"
#include <stdlib.h>
#include <string.h>
#include <wwd_assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DTLS_MAX_BUF        100
#define NONCE_SIZE          16
#define DTLS_EC_KEY_SIZE    32
#define DTLS_RH_LENGTH sizeof(dtls_record_header_t)
#define DTLS_HS_LENGTH sizeof(dtls_handshake_header_t)
#define DTLS_CH_LENGTH sizeof(dtls_client_hello_t) /* no variable length fields! */
#define DTLS_COOKIE_LENGTH_MAX 32
#define DTLS_CH_LENGTH_MAX sizeof(dtls_client_hello_t) + DTLS_COOKIE_LENGTH_MAX + 12 + 26
#define DTLS_HV_LENGTH sizeof(dtls_hello_verify_t)
#define DTLS_SH_LENGTH (2 + DTLS_RANDOM_LENGTH + 1 + 2 + 1)
#define DTLS_CE_LENGTH (3 + 3 + 27 + DTLS_EC_KEY_SIZE + DTLS_EC_KEY_SIZE)
#define DTLS_SKEXEC_LENGTH (1 + 2 + 1 + 1 + DTLS_EC_KEY_SIZE + DTLS_EC_KEY_SIZE + 1 + 1 + 2 + 70)
#define DTLS_SKEXECPSK_LENGTH_MIN 2
#define DTLS_SKEXECPSK_LENGTH_MAX 2 + DTLS_PSK_MAX_CLIENT_IDENTITY_LEN
#define DTLS_CKXEC_LENGTH (1 + 1 + DTLS_EC_KEY_SIZE + DTLS_EC_KEY_SIZE)
#define DTLS_CKXPSK_LENGTH_MIN 2
#define DTLS_FIN_LENGTH 12

#define DTLS_RECORD_HEADER(M) ((dtls_record_header_t *)(M))
#define DTLS_HANDSHAKE_HEADER(M) ((dtls_handshake_header_t *)(M))

#define DTLS_CT_CHANGE_CIPHER_SPEC 20
#define DTLS_CT_ALERT              21
#define DTLS_CT_HANDSHAKE          22
#define DTLS_CT_APPLICATION_DATA   23

/* Handshake types */
#define DTLS_HT_HELLO_REQUEST        0
#define DTLS_HT_CLIENT_HELLO         1
#define DTLS_HT_SERVER_HELLO         2
#define DTLS_HT_HELLO_VERIFY_REQUEST 3
#define DTLS_HT_CERTIFICATE         11
#define DTLS_HT_SERVER_KEY_EXCHANGE 12
#define DTLS_HT_CERTIFICATE_REQUEST 13
#define DTLS_HT_SERVER_HELLO_DONE   14
#define DTLS_HT_CERTIFICATE_VERIFY  15
#define DTLS_HT_CLIENT_KEY_EXCHANGE 16
#define DTLS_HT_FINISHED            20

/**
 * Maximum size of the generated keyblock. Note that MAX_KEYBLOCK_LENGTH must
 * be large enough to hold the pre_master_secret, i.e. twice the length of the
 * pre-shared key + 1.
 */
#define MAX_KEYBLOCK_LENGTH  \
  (2 * DTLS_MAC_KEY_LENGTH + 2 * DTLS_KEY_LENGTH + 2 * DTLS_IV_LENGTH)

#define DTLS_MASTER_SECRET_LENGTH 48
#define DTLS_RANDOM_LENGTH 32
#define DTLS_PSK_MAX_CLIENT_IDENTITY_LEN   32
#define DTLS_PSK_MAX_KEY_LEN 32
#define DTLS_HASH_LENGTH     36

/* The following macros provide access to different keys from Key-Block */
#define dtls_kb_remote_mac_secret(Param, Role)  ((Role) == DTLS_SERVER ? dtls_kb_client_mac_secret(Param, Role) : dtls_kb_server_mac_secret(Param, Role))
#define dtls_kb_local_mac_secret(Param, Role)   ((Role) == DTLS_CLIENT ? dtls_kb_client_mac_secret(Param, Role) : dtls_kb_server_mac_secret(Param, Role))
#define dtls_kb_remote_write_key(Param, Role)   ((Role) == DTLS_SERVER ? dtls_kb_client_write_key(Param, Role) : dtls_kb_server_write_key(Param, Role))
#define dtls_kb_local_write_key(Param, Role)    ((Role) == DTLS_CLIENT ? dtls_kb_client_write_key(Param, Role) : dtls_kb_server_write_key(Param, Role))
#define dtls_kb_remote_iv(Param, Role)          ((Role) == DTLS_SERVER ? dtls_kb_client_iv(Param, Role)        : dtls_kb_server_iv(Param, Role))
#define dtls_kb_local_iv(Param, Role)           ((Role) == DTLS_CLIENT ? dtls_kb_client_iv(Param, Role)        : dtls_kb_server_iv(Param, Role))
#define dtls_kb_iv_size(Param, Role)            ( DTLS_IV_LENGTH )
#define dtls_kb_mac_secret_size(Param, Role)    ( DTLS_MAC_KEY_LENGTH )
#define dtls_kb_client_mac_secret(Param, Role)  ((Param)->key_block)
#define dtls_kb_key_size(Param, Role)           ( DTLS_KEY_LENGTH )
#define dtls_kb_client_iv(Param, Role)          (dtls_kb_server_write_key(Param, Role)  + DTLS_KEY_LENGTH)
#define dtls_kb_server_iv(Param, Role)          (dtls_kb_client_iv(Param, Role)         + DTLS_IV_LENGTH)
#define dtls_kb_client_write_key(Param, Role)   (dtls_kb_server_mac_secret(Param, Role) + DTLS_MAC_KEY_LENGTH)
#define dtls_kb_server_write_key(Param, Role)   (dtls_kb_client_write_key(Param, Role)  + DTLS_KEY_LENGTH)

#define dtls_kb_size(Param, Role)               (2 * (dtls_kb_mac_secret_size(Param, Role) +  dtls_kb_key_size(Param, Role) + dtls_kb_iv_size(Param, Role)))
#define dtls_kb_server_mac_secret(Param, Role)  (dtls_kb_client_mac_secret(Param, Role) + DTLS_MAC_KEY_LENGTH)

/* just for consistency */
#define dtls_kb_digest_size(Param, Role) DTLS_MAC_LENGTH

typedef enum
{
    DTLS_ALERT_LEVEL_WARNING = 1,
    DTLS_ALERT_LEVEL_FATAL = 2
} dtls_alert_level_t;

typedef enum
{
    DTLS_ALERT_CLOSE_NOTIFY             = 0,      /* close_notify */
    DTLS_ALERT_UNEXPECTED_MESSAGE       = 10,     /* unexpected_message */
    DTLS_ALERT_BAD_RECORD_MAC           = 20,     /* bad_record_mac */
    DTLS_ALERT_RECORD_OVERFLOW          = 22,     /* record_overflow */
    DTLS_ALERT_DECOMPRESSION_FAILURE    = 30,     /* decompression_failure */
    DTLS_ALERT_HANDSHAKE_FAILURE        = 40,     /* handshake_failure */
    DTLS_ALERT_BAD_CERTIFICATE          = 42,     /* bad_certificate */
    DTLS_ALERT_UNSUPPORTED_CERTIFICATE  = 43,     /* unsupported_certificate */
    DTLS_ALERT_CERTIFICATE_REVOKED      = 44,     /* certificate_revoked */
    DTLS_ALERT_CERTIFICATE_EXPIRED      = 45,     /* certificate_expired */
    DTLS_ALERT_CERTIFICATE_UNKNOWN      = 46,     /* certificate_unknown */
    DTLS_ALERT_ILLEGAL_PARAMETER        = 47,     /* illegal_parameter */
    DTLS_ALERT_UNKNOWN_CA               = 48,     /* unknown_ca */
    DTLS_ALERT_ACCESS_DENIED            = 49,     /* access_denied */
    DTLS_ALERT_DECODE_ERROR             = 50,     /* decode_error */
    DTLS_ALERT_DECRYPT_ERROR            = 51,     /* decrypt_error */
    DTLS_ALERT_PROTOCOL_VERSION         = 70,     /* protocol_version */
    DTLS_ALERT_INSUFFICIENT_SECURITY    = 71,     /* insufficient_security */
    DTLS_ALERT_INTERNAL_ERROR           = 80,     /* internal_error */
    DTLS_ALERT_USER_CANCELED            = 90,     /* user_canceled */
    DTLS_ALERT_NO_RENEGOTIATION         = 100,    /* no_renegotiation */
    DTLS_ALERT_UNSUPPORTED_EXTENSION    = 110
/* unsupported_extension */
} dtls_alert_t;

#pragma pack(1)

/** Structure of the Client Hello message. */
typedef struct
{
        uint16_t        version;        /**< Client version */
        uint32_t        gmt_random;     /**< GMT time of the random byte creation */
        unsigned char   random[ 28 ];   /**< Client random bytes */
        /* session id (up to 32 bytes) */
        /* cookie (up to 32 bytes) */
        /* cipher suite (2 to 2^16 -1 bytes) */
        /* compression method */
} dtls_client_hello_t;
#pragma pack()

int dtls_encrypt( unsigned char *src, size_t length, unsigned char *buf, unsigned char *nounce, unsigned char *key, size_t keylen, const unsigned char *aad, size_t aad_length);
int dtls_decrypt(const unsigned char *src, size_t length,unsigned char *buf, unsigned char *nounce, unsigned char *key, size_t keylen, const unsigned char *a_data, size_t a_data_length);
dtls_handshake_parameters_t *dtls_handshake_new();
void dtls_handshake_free(dtls_handshake_parameters_t *handshake);
dtls_security_parameters_t *dtls_security_new();
void dtls_security_free(dtls_security_parameters_t *security);

static inline dtls_security_parameters_t *dtls_security_params_next( dtls_peer_t *peer )
{
    if ( peer->security_params[ 1 ] )
        dtls_security_free( peer->security_params[ 1 ] );

    peer->security_params[ 1 ] = dtls_security_new( );
    if ( !peer->security_params[ 1 ] )
    {
        return NULL;
    }
    peer->security_params[ 1 ]->epoch = peer->security_params[ 0 ]->epoch + 1;
    return peer->security_params[ 1 ];
}

static inline void dtls_security_params_free_other( dtls_peer_t *peer )
{
    dtls_security_parameters_t * security0 = peer->security_params[ 0 ];
    dtls_security_parameters_t * security1 = peer->security_params[ 1 ];

    if ( !security0 || !security1 || security0->epoch < security1->epoch )
        return;

    dtls_security_free( security1 );
    peer->security_params[ 1 ] = NULL;
}

static inline dtls_security_parameters_t *dtls_security_params_epoch( dtls_peer_t *peer, uint16_t epoch )
{
    if ( peer->security_params[ 0 ] && peer->security_params[ 0 ]->epoch == epoch )
    {
        return peer->security_params[ 0 ];
    }
    else if ( peer->security_params[ 1 ] && peer->security_params[ 1 ]->epoch == epoch )
    {
        return peer->security_params[ 1 ];
    }
    else
    {
        return NULL;
    }
}

static inline void dtls_security_params_switch( dtls_peer_t *peer )
{
    dtls_security_parameters_t * security = peer->security_params[ 1 ];

    peer->security_params[ 1 ] = peer->security_params[ 0 ];
    peer->security_params[ 0 ] = security;
}

static inline dtls_security_parameters_t *dtls_security_params( dtls_peer_t *peer )
{
    return peer->security_params[ 0 ];
}


/* Function Used in forming DTLS packet */

/* this one is for consistency... */
static inline int dtls_int_to_uint8( unsigned char *field, uint8_t value )
{
    field[ 0 ] = value & 0xff;
    return 1;
}

static inline int dtls_int_to_uint16( unsigned char *field, uint16_t value )
{
    field[ 0 ] = ( value >> 8 ) & 0xff;
    field[ 1 ] = value & 0xff;
    return 2;
}

static inline int dtls_int_to_uint24( unsigned char *field, uint32_t value )
{
    field[ 0 ] = ( value >> 16 ) & 0xff;
    field[ 1 ] = ( value >> 8 ) & 0xff;
    field[ 2 ] = value & 0xff;
    return 3;
}

static inline int dtls_int_to_uint32( unsigned char *field, uint32_t value )
{
    field[ 0 ] = ( value >> 24 ) & 0xff;
    field[ 1 ] = ( value >> 16 ) & 0xff;
    field[ 2 ] = ( value >> 8 ) & 0xff;
    field[ 3 ] = value & 0xff;
    return 4;
}

static inline int dtls_int_to_uint48( unsigned char *field, uint64_t value )
{
    field[ 0 ] = ( value >> 40 ) & 0xff;
    field[ 1 ] = ( value >> 32 ) & 0xff;
    field[ 2 ] = ( value >> 24 ) & 0xff;
    field[ 3 ] = ( value >> 16 ) & 0xff;
    field[ 4 ] = ( value >> 8 ) & 0xff;
    field[ 5 ] = value & 0xff;
    return 6;
}

static inline int dtls_int_to_uint64( unsigned char *field, uint64_t value )
{
    field[ 0 ] = ( value >> 56 ) & 0xff;
    field[ 1 ] = ( value >> 48 ) & 0xff;
    field[ 2 ] = ( value >> 40 ) & 0xff;
    field[ 3 ] = ( value >> 32 ) & 0xff;
    field[ 4 ] = ( value >> 24 ) & 0xff;
    field[ 5 ] = ( value >> 16 ) & 0xff;
    field[ 6 ] = ( value >> 8 ) & 0xff;
    field[ 7 ] = value & 0xff;
    return 8;
}

static inline uint8_t dtls_uint8_to_int( const unsigned char *field )
{
    return (uint8_t) field[ 0 ];
}

static inline uint16_t dtls_uint16_to_int( const unsigned char *field )
{
    return ( (uint16_t) field[ 0 ] << 8 ) | (uint16_t) field[ 1 ];
}

static inline uint32_t dtls_uint24_to_int( const unsigned char *field )
{
    return ( (uint32_t) field[ 0 ] << 16 ) | ( (uint32_t) field[ 1 ] << 8 ) | (uint32_t) field[ 2 ];
}

static inline uint32_t dtls_uint32_to_int( const unsigned char *field )
{
    return ( (uint32_t) field[ 0 ] << 24 ) | ( (uint32_t) field[ 1 ] << 16 ) | ( (uint32_t) field[ 2 ] << 8 ) | (uint32_t) field[ 3 ];
}

static inline uint64_t dtls_uint48_to_int( const unsigned char *field )
{
    return ( (uint64_t) field[ 0 ] << 40 ) | ( (uint64_t) field[ 1 ] << 32 ) | ( (uint64_t) field[ 2 ] << 24 ) | ( (uint64_t) field[ 3 ] << 16 ) | ( (uint64_t) field[ 4 ] << 8 ) | (uint64_t) field[ 5 ];
}

static inline uint64_t dtls_uint64_to_int( const unsigned char *field )
{
    return ( (uint64_t) field[ 0 ] << 56 ) | ( (uint64_t) field[ 1 ] << 48 ) | ( (uint64_t) field[ 2 ] << 40 ) | ( (uint64_t) field[ 3 ] << 32 ) | ( (uint64_t) field[ 4 ] << 24 ) | ( (uint64_t) field[ 5 ] << 16 ) | ( (uint64_t) field[ 6 ] << 8 ) | (uint64_t) field[ 7 ];
}

void dtls_cleanup_handshake_message( dtls_context_t* dtls, dtls_handshake_message_t* message );
void dtls_cleanup_current_record( dtls_context_t* dtls );
void dtls_free_peer( dtls_peer_t *peer );
void dtls_add_peer( dtls_context_t *ctx, dtls_peer_t *peer );
void dtls_free( dtls_context_t* context, dtls_session_t* session );

int dtls_session_equals( const dtls_session_t *a, const dtls_session_t *b );
int decrypt_verify( dtls_context_t* dtls, dtls_session_t* session, dtls_record_t* record, size_t length, uint8_t **cleartext );
uint8_t* dtls_set_handshake_header( uint8_t type, dtls_context_t* ctx, dtls_peer_t *peer, int length, int frag_offset, int frag_length, uint8_t *buf );
uint64_t dtls_host_get_time_ms( void );
int dtls_prepare_record( dtls_peer_t* peer, dtls_context_t* context, dtls_security_parameters_t* security, unsigned char type, dtls_record_t *data, size_t len, int need_encrypt );

dtls_result_t dtls_write_alert_msg( dtls_context_t* context, int level, int description );
dtls_result_t dtls_send_multi( dtls_context_t *ctx, dtls_peer_t *peer, dtls_security_parameters_t *security, dtls_session_t *session, unsigned char type, dtls_record_t *buf, size_t buf_len, int value );
dtls_peer_t*  dtls_new_peer( const dtls_session_t *session );
dtls_result_t dtls_send_handshake_msg_hash( dtls_context_t *ctx, dtls_peer_t *peer, dtls_session_t *session, uint8_t header_type, dtls_record_t *data, size_t data_length, int add_hash, int value );
dtls_result_t dtls_send( dtls_context_t *ctx, dtls_peer_t *peer, unsigned char type, dtls_record_t* record, size_t buflen, int value );
dtls_result_t calculate_key_block( dtls_context_t *ctx, dtls_handshake_parameters_t *handshake, dtls_peer_t *peer );
dtls_result_t dtls_skip_current_record( dtls_context_t* context );
//dtls_result_t dtls_get_next_record( dtls_context_t* dtls, dtls_session_t* session, dtls_record_t** record, uint32_t timeout, dtls_packet_receive_option_t packet_receive_option );
dtls_result_t dtls_get_next_handshake_message( dtls_context_t* dtls, dtls_session_t* session, dtls_handshake_message_t** message, uint16_t* message_length );
dtls_peer_t   *dtls_get_peer( dtls_context_t *context, const dtls_session_t *session );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* _DTLS_DTLS_H_ */
