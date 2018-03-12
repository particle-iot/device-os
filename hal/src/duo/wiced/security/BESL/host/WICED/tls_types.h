/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "besl_structures.h"
#include "crypto_structures.h"
#include <time.h>
#include "cipher_suites.h"
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SIZEOF_SESSION_ID                                   32
#define SIZEOF_SESSION_MASTER                               48
#define SIZEOF_RANDOM                                       64

#define  TLS_WAIT_FOREVER                                  (0xFFFFFFFF)
#define  TLS_HANDSHAKE_PACKET_TIMEOUT_MS                   (20000)

/* Max negotiated length is SSL_MAX_FRAGMENT_LEN_4096 [4K], But if not negotiated then TLS_DEFAULT_MAX_FRAGMENT_LENGTH [16K]
 * will be used. Its not possible to negotiate above SSL_MAX_FRAGMENT_LEN_4096 [4K].
 */
#define  TLS_DEFAULT_MAX_FRAGMENT_LENGTH                   (16384)
#define  TLS_EXT_MAX_FRAGMENT_LENGTH_FIELD_SIZE            (1)
#define  TLS_MSG_APPLICATION_DATA                          (23)

#define WICED_TLS_CONTEXT_ID                               (0xd309c08b)
/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    TLS_NO_VERIFICATION       = 0,
    TLS_VERIFICATION_OPTIONAL = 1,
    TLS_VERIFICATION_REQUIRED = 2,
} wiced_tls_certificate_verification_t;

typedef enum
{
    WICED_TLS_SIMPLE_CONTEXT,
    WICED_TLS_ADVANCED_CONTEXT,
} wiced_tls_context_type_t;

typedef enum
{
    WICED_TLS_AS_CLIENT = 0,
    WICED_TLS_AS_SERVER = 1
} wiced_tls_endpoint_type_t;

/*
 * SSL state machine
 */
typedef enum
{
    SSL_HELLO_REQUEST,
    SSL_CLIENT_HELLO,
    SSL_SERVER_HELLO,
    SSL_SERVER_CERTIFICATE,
    SSL_SERVER_KEY_EXCHANGE,
    SSL_CERTIFICATE_REQUEST,
    SSL_SERVER_HELLO_DONE,
    SSL_CLIENT_CERTIFICATE,
    SSL_CLIENT_KEY_EXCHANGE,
    SSL_CERTIFICATE_VERIFY,
    SSL_CLIENT_CHANGE_CIPHER_SPEC,
    SSL_CLIENT_FINISHED,
    SSL_SERVER_CHANGE_CIPHER_SPEC,
    SSL_SERVER_FINISHED,
    SSL_FLUSH_BUFFERS,
    SSL_HANDSHAKE_OVER
} tls_states_t;

/*
 * TLS transport protocol types
 */
typedef enum
{
    TLS_TCP_TRANSPORT = 0,
    TLS_EAP_TRANSPORT = 1,
    TLS_UDP_TRANSPORT = 2,
} tls_transport_protocol_t;

typedef enum
{
    TLS_RECEIVE_PACKET_IF_NEEDED,
    TLS_AVOID_NEW_RECORD_PACKET_RECEIVE,
} tls_packet_receive_option_t;

typedef enum
{
    TLS_CERTIFICATE_IN_PEM_FORMAT,
    TLS_CERTIFICATE_IN_DER_FORMAT,
} wiced_tls_certificate_format_t;

typedef enum
{
    TLS_DIGITALLY_SIGNED_SIGNATURE_ALGORITHM_ANONYMOUS = 0,
    TLS_DIGITALLY_SIGNED_SIGNATURE_ALGORITHM_RSA       = 1,
    TLS_DIGITALLY_SIGNED_SIGNATURE_ALGORITHM_DSA       = 2,
    TLS_DIGITALLY_SIGNED_SIGNATURE_ALGORITHM_ECDSA     = 3,
} tls_digitally_signed_signature_algorithm_t;

/* Reference: http://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml */
typedef enum
{
    TLS_EXTENSION_TYPE_SERVER_NAME                              =  0,
    TLS_EXTENSION_TYPE_MAX_FRAGMENT_LENGTH                      =  1,
    TLS_EXTENSION_TYPE_APPLICATION_LAYER_PROTOCOL_NEGOTIATION   = 16,
} wiced_tls_extension_type_t;

typedef enum
{
    TLS_FRAGMENT_LENGTH_512   =  1,
    TLS_FRAGMENT_LENGTH_1024  =  2,
    TLS_FRAGMENT_LENGTH_2048  =  3,
    TLS_FRAGMENT_LENGTH_4096  =  4,
} wiced_tls_max_fragment_length_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct _ssl_context  wiced_tls_workspace_t;
typedef struct _ssl_session  wiced_tls_session_t;
typedef uint32_t             tls_packet_t;

typedef int (*wiced_tls_sign_certificate_verify)(  wiced_tls_rsa_key_t* rsa_key ,rsa_hash_id_t hash_id, int32_t hashlen, const unsigned char *hash, unsigned char *rsa_sign, uint32_t* key_length );

typedef struct
{
    const uint8_t*   certificate_data;
    uint32_t         certificate_data_length;
    wiced_bool_t     certificate_data_malloced;
    wiced_tls_key_t* public_key;
    const char*      common_name;
    x509_cert*       processed_certificate_data;
} wiced_tls_certificate_t;

typedef struct
{
    union
    {
        wiced_tls_key_t     common;
        wiced_tls_rsa_key_t rsa;
        wiced_tls_ecc_key_t ecc;
        wiced_tls_psk_key_t psk;
    } private_key;
    wiced_tls_certificate_t certificate;
    wiced_tls_sign_certificate_verify rsa_sign;
} wiced_tls_identity_t;

#pragma pack(1)

/* Helper structure to create TLS record */

typedef struct
{
    uint8_t  type;
    uint8_t  major_version;
    uint8_t  minor_version;
    uint16_t length;
} tls_record_header_t;

typedef struct
{
    uint8_t  type;
    uint8_t  major_version;
    uint8_t  minor_version;
    uint16_t length;
    uint8_t  message[1];
} tls_record_t;

typedef struct
{
    uint8_t  record_type;
    uint8_t  major_version;
    uint8_t  minor_version;
    uint16_t record_length;
    uint8_t  handshake_type;
    uint8_t  handshake_length[3];
    uint8_t  content[1];
} tls_handshake_record_t;

typedef struct
{
    uint8_t  type;
    uint8_t  length[3];
    uint8_t  data[1];
} tls_handshake_message_t;

typedef struct
{
    uint8_t  type;
    uint8_t  length[3];
} tls_handshake_header_t;

typedef struct
{
    uint8_t  record_type;
    uint8_t  major_version;
    uint8_t  minor_version;
    uint16_t record_length;
    uint8_t  handshake_type;
    uint8_t  handshake_length[3];
    uint8_t  certificate_length[3];
    uint8_t  certificate_data[1];
} tls_certificate_record_t;

#pragma pack()

typedef struct _ssl_session ssl_session;
typedef struct _ssl_context ssl_context;

struct _ssl_session
{
    time_t start;                                 /*!< starting time      */
    const cipher_suite_t* cipher;                 /*!< chosen cipher      */
    int32_t length;                               /*!< session id length  */
    unsigned char id[SIZEOF_SESSION_ID];          /*!< session identifier */
    unsigned char master[SIZEOF_SESSION_MASTER];  /*!< the master secret  */
    ssl_session *next;                            /*!< next session entry */
    void *appdata;                                /*!< application extension */
    int32_t age;
};

typedef union
{
        arc4_context arc4;
        des3_context des3;
        aes_context_t aes;
        camellia_context camellia;
        chacha_context_t chacha;
        seed_context_t seed;
        void* gcm;
        void* ccm;
} tls_cipher_context;

/*
 * This structure is used for extensions
 */
#define MAX_NUMBER_OF_EXTENSIONS_SUPPORTED       8
#define MAX_EXT_DATA_LENGTH                      256
struct _ssl_extension
{
    uint16_t id;
    uint16_t used;
    uint16_t sz;
    uint8_t data[MAX_EXT_DATA_LENGTH+1];
};

typedef struct _ssl_extension ssl_extension;

struct _ssl_context
{
    /*
     * Miscellaneous
     */
    int32_t state;                  /*!< SSL handshake: current state     */

    int32_t major_ver;              /*!< equal to  SSL_MAJOR_VERSION_3    */
    int32_t minor_ver;              /*!< either 0 (SSL3) or 1 (TLS1.0)    */

    int32_t max_major_ver;          /*!< max. major version from client   */
    int32_t max_minor_ver;          /*!< max. minor version from client   */

    /*
     * Callbacks (RNG, debug, I/O)
     */
    int32_t  (*f_rng)(void *);
    void (*f_dbg)(void *, int32_t, char *);

    void *p_rng;                /*!< context for the RNG function     */
    void *p_dbg;                /*!< context for the debug function   */

    void* receive_context;            /*!< context for reading operations   */
    void* send_context;               /*!< context for writing operations   */

    /*
     * Session layer
     */
    int32_t resume;                         /*!<  session resuming flag   */
    int32_t timeout;                    /*!<  sess. expiration time   */
    ssl_session *session;               /*!<  current session data    */
    int32_t (*get_session)(ssl_context*);        /*!<  (server) get callback   */
    int32_t (*set_session)(ssl_context*);        /*!<  (server) set callback   */

    /*
     * Record layer (incoming data)
     */
    uint8_t*      defragmentation_buffer;
    uint16_t      defragmentation_buffer_length;
    uint16_t      defragmentation_buffer_bytes_processed;
    uint16_t      defragmentation_buffer_bytes_received;

    tls_packet_t* received_packet;
    uint16_t      received_packet_length;
    uint16_t      received_packet_bytes_skipped;
    tls_record_t* current_record;
    uint16_t      current_record_bytes_processed;
    uint16_t      current_record_original_length;

    tls_transport_protocol_t transport_protocol;

    tls_handshake_message_t* current_handshake_message;

    unsigned char  in_ctr[8];      /*!< 64-bit incoming message counter  */

    int32_t nb_zero;                /*!< # of 0-length encrypted messages */

    /*
     * Record layer (outgoing data)
     */
    unsigned char out_ctr[8];     /*!< 64-bit outgoing message counter  */

    tls_packet_t* outgoing_packet;

    int32_t out_msgtype;            /*!< record header: message type      */
    uint32_t out_buffer_size; /* The maximum amount that can be written to the current buffer */

    /*
     * PKI layer
     */
    wiced_tls_identity_t* identity;
    x509_cert *ca_chain;                /*!<  own trusted CA chain    */
    x509_cert *peer_cert;               /*!<  peer X.509 cert chain   */
    const char *peer_cn;                /*!<  expected peer CN        */
    wiced_tls_key_t* peer_public_key;

    int32_t endpoint;                   /*!<  0: client, 1: server    */
    int32_t authmode;                   /*!<  verification mode       */
    int32_t client_auth;                /*!<  flag for client auth.   */
    int32_t verify_result;              /*!<  verification result     */

    /*
     * Crypto layer
     */
    dhm_context dhm_ctx;                /*!<  DHM key exchange        */
    md5_context fin_md5;                /*!<  Finished MD5 checksum   */
    sha1_context fin_sha1;              /*!<  Finished SHA-1 checksum */
    sha2_context fin_sha2;              /*!<  Finished SHA-2 checksum */

    int32_t do_crypt;                   /*!<  en(de)cryption flag     */
    const cipher_suite_t** ciphers;     /*!<  allowed ciphersuites    */
    uint32_t pmslen;                    /*!<  premaster length        */
    int32_t keylen;                     /*!<  symmetric key length    */
    int32_t minlen;                     /*!<  min. ciphertext length  */
    int32_t ivlen;                      /*!<  IV length               */
    int32_t iv_fixedlen;                /*!<  Lenght of fixed part of IV */
    int32_t maclen;                     /*!<  MAC length              */
    int verifylen;                      /*!<  verify data length      */

    unsigned char randbytes[64];        /*!<  random bytes            */
    unsigned char premaster[256];       /*!<  premaster secret        */

    unsigned char iv_enc[16];           /*!<  IV (encryption)         */
    unsigned char iv_dec[16];           /*!<  IV (decryption)         */

    unsigned char mac_enc[32];          /*!<  MAC (encryption)        */
    unsigned char mac_dec[32];          /*!<  MAC (decryption)        */

    tls_cipher_context  ctx_enc;              /*!<  encryption context      */
    tls_cipher_context  ctx_dec;              /*!<  decryption context      */

    /*
     * TLS extensions
     */
    uint16_t ext_max_fragment_length;
    int extension_count;
    ssl_extension extensions[MAX_NUMBER_OF_EXTENSIONS_SUPPORTED];
};

typedef struct
{
    uint32_t                 context_id;
    wiced_tls_workspace_t    context;
    wiced_tls_session_t      session;
    wiced_tls_identity_t*    identity;
} wiced_tls_context_t;

typedef struct
{
    wiced_tls_extension_type_t           type;
    union
    {
        uint8_t*                         server_name;
        wiced_tls_max_fragment_length_t  max_fragment_length;
    } extension_data;
} wiced_tls_extension_t;


typedef enum
{
    TLS_RESULT_LIST     (  TLS_      )  /* 5000 - 5999 */
} tls_result_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

tls_result_t ssl_init                  ( ssl_context *ssl );
void         ssl_set_rng               ( ssl_context *ssl, int32_t (*f_rng)( void * ), void *p_rng );
void         ssl_set_session           ( ssl_context *ssl, int32_t resume, int32_t timeout, ssl_session *session );
void         ssl_free                  ( ssl_context *ssl );
void         ssl_set_endpoint          ( ssl_context *ssl, int32_t endpoint );
void         ssl_set_ca_chain          ( ssl_context *ssl, x509_cert *ca_chain, const char *peer_cn );
void         ssl_set_authmode          ( ssl_context *ssl, int32_t authmode );
void         ssl_set_own_cert          ( ssl_context *ssl, x509_cert *own_cert, rsa_context *rsa_key );
tls_result_t ssl_handshake_server_async( ssl_context *ssl );
tls_result_t ssl_handshake_client_async( ssl_context *ssl );
tls_result_t ssl_set_dh_param          ( ssl_context *ssl, const unsigned char *dhm_P, int P_len, const unsigned char *dhm_G, int G_len );
tls_result_t ssl_close_notify          ( ssl_context *ssl );
tls_result_t tls_encrypt_record        ( ssl_context *ssl, tls_record_t* record, uint32_t message_length );
tls_result_t tls_decrypt_record        ( ssl_context *ssl, tls_record_t* record );
void         microrng_init             ( microrng_state *state );
int32_t      microrng_rand             ( void *p_state );
int          ussl_set_extension        ( ssl_context *ssl, const ssl_extension *extension );


#ifdef __cplusplus
} /*extern "C" */
#endif
