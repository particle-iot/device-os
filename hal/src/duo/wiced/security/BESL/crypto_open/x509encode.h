/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */

#include "x509.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/** Create x.509 ASN.1 DER tag
 *
 * @param tag : tag id
 * @param len : length tag value, @param val
 * @param val : tag value
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_tag( uint8_t tag, size_t len, unsigned char * val, size_t * total_size );

/** Create x.509 ASN.1 DER version tag
 *
 * @param ver : '2' for version 3
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated version tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_version( int8_t ver, size_t * total_size );

/** Create x.509 ASN.1 DER serial number tag
 *
 * @param serial : serial number in big endian big integer number
 * @param len : size of serial number
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_serial_number( void * serial, size_t serial_size, size_t * total_size );

/** Create x.509 ASN.1 DER set alorithm tag
 *
 * @param algo : algorithm object id
 * @param len : length of algorithm object id
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_algorithm_id( void * algo, size_t algo_size, size_t * total_size );

/** Create x.509 ASN.1 DER name object tag and value
 *
 * @param object_id : name object id
 * @param object_sz : size of name objec id
 * @param value_tag : value type tag
 * @param value : value data
 * @param value_sz : size of value data
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_name_type_value( void * object_id, size_t object_sz,
                                             uint8_t value_tag, void * value, size_t value_sz,
                                             size_t * total_size );

/** Create x.509 ASN.1 DER name tag
 *
 * @param C : country value
 * @param cz_C : size of country value
 * @param ST : state value
 * @param cz_ST : size of state value
 * @param L : locality value
 * @param cz_L : size of locality value
 * @param O : organization value
 * @param cz_O : size of organization value
 * @param OU : organization unit value
 * @param cz_OU : size of organization unit value
 * @param CN : common name value
 * @param cz_CN : size of common name value
 * @param E : email value
 * @param cz_E : size of email value
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_name( void * C, size_t sz_C,
                                  void * ST, size_t sz_ST,
                                  void * L, size_t sz_L,
                                  void * O, size_t sz_O,
                                  void * OU, size_t sz_OU,
                                  void * CN, size_t sz_CN,
                                  void * E, size_t sz_E,
                                  size_t * total_size );

/** Create x.509 ASN.1 DER date and time tag
 *
 * @param y : year
 * @param m : month
 * @param d : day
 * @param h : hour
 * @param min : minute
 * @param sec : second
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_time( uint32_t y, uint8_t m, uint8_t d,
                                  uint8_t h, uint8_t min, uint8_t sec,
                                  size_t * total_size );

/** Create x.509 ASN.1 DER validity tag
 *
 * @param s_y : not before year
 * @param s_m : not before month
 * @param s_d : not before day
 * @param s_h : not before hour
 * @param s_min : not before minute
 * @param s_sec : not before second
 * @param e_y : not after year
 * @param e_m : not after month
 * @param e_d : not after day
 * @param e_h : not after hour
 * @param e_min : not after minute
 * @param e_sec : not after second
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_validity( uint32_t s_y, uint8_t s_m, uint8_t s_d,
                                      uint8_t s_h, uint8_t s_min, uint8_t s_sec,
                                      uint32_t e_y, uint8_t e_m, uint8_t e_d,
                                      uint8_t e_h, uint8_t e_min, uint8_t e_sec,
                                      size_t * total_size );

/** Create x.509 ASN.1 DER public key tag
 *
 * @param pubkey : public key modulus
 * @param pubkey_size : size of public key modulus
 * @param pubkey : public key exponent
 * @param pubkey_size : size of public key exponent
 * @param sha1hash : calulated DER format public key's sha1 hash
 *                   which is used for key identifier
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_pubkey( void * pubkey, size_t pubkey_size,
                                    void * pubexp, size_t pubexp_size,
                                    unsigned char sha1hash[20],
                                    size_t * total_size );

/** Create x.509 ASN.1 DER public key information tag
 *
 * @param algo : algorithm id of public key
 * @param len : length of algorithm object id
 * @param pubkey : public key modulus
 * @param pubkey_size : size of public key modulus
 * @param pubkey : public key exponent
 * @param pubkey_size : size of public key exponent
 * @param sha1hash : calulated DER format public key's sha1 hash
 *                   which is used for key identifier
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_subject_pubkey_info( void * algo, size_t algo_size,
                                                 void * pubkey, size_t pubkey_size,
                                                 void * pubexp, size_t pubexp_size,
                                                 unsigned char sha1hash[20], size_t * total_size );

/** Create x.509 ASN.1 DER subject key id tag for x.509 extension v3
 *
 * @param key_sha1hash : sha1hash of subject key
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_subject_key_id( unsigned char key_sha1hash[20], size_t * total_size );

/** Create x.509 ASN.1 DER authority key id tag for x.509 extension v3
 *
 * @param key_sha1hash : sha1hash of authority key
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_authority_key_id( unsigned char key_sha1hash[20], size_t * total_size );

/** Create x.509 ASN.1 DER CA value in basic constraints for x.509 extension v3
 *
 * @param ca : ca value. 0 for FALSE or TRUE
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_basic_constraints( uint8_t ca, size_t * total_size );

/** Create x.509 ASN.1 DER x.509 extension v3 tag
 *
 * @param subkey_sha1hash : sha1hash of subject key
 * @param authkey_sha1hash : sha1hash of authority key
 * @param ca : ca value. 0 for FALSE or TRUE
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_extensions( unsigned char subkey_sha1hash[20],
                                        unsigned char authkey_sha1hash[20],
                                        uint8_t ca, size_t * total_size );

/** Create x.509 ASN.1 DER tag
 *
 * @param tag : tag id
 * @param len : length tag value, @param val
 * @param val : tag value
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_signature( unsigned char * signature,
                                       size_t signature_size,
                                       size_t * total_size );

/** Create x.509 ASN.1 DER tbs certificate tag
 *
 * @param ver : version in DER format
 * @param sz_ver : size of ver
 * @param sn : serial number in DER format
 * @param sz_sn : size of sn
 * @param issuer : issuer name in DER format
 * @param sz_issuer : size of issuer
 * @param validity : validity in DER format
 * @param sz_validity : size of validity
 * @param subject : subject name in DER format
 * @param sz_subject : size of subject
 * @param pubkey_info : subject public key info in DER format
 * @param sz_sn : size of pubkey_info
 * @param ext : x.509 ext v3 in DER format
 * @param sz_sn : size of ext
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_gen_tbs_cert( void * ver, size_t sz_ver,
                                      void * sn, size_t sz_sn,
                                      void * algo, size_t sz_algo,
                                      void * issuer, size_t sz_issuer,
                                      void * validity, size_t sz_validity,
                                      void * subject, size_t sz_subject,
                                      void * pubkey_info, size_t sz_pubkey_info,
                                      void * ext, size_t sz_ext,
                                      size_t * total_size );

/** Create x.509 ASN.1 DER certificate tag
 *
 * @param tbs_cert : tbs certificate in DER format
 * @param sz_tbs_cert : size of tbs_cert
 * @param algo : algoritm
 * @param sz_tbs_cert : size of tbs_cert
 * @param algo : algorithm id of signature
 * @param len : length of algorithm object id
 * @param signature : signature value
 * @param len : size of signature
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_gen_certificate( void * tbs_cert, size_t sz_tbs_cert,
                                         void * algo, size_t sz_algo,
                                         void * signature, size_t sz_signature,
                                         size_t * total_size );

/** convert x.509 cert DER to PEM
 *
 * @param der : DER format certificate
 * @param sz_der : size of der
 * @param conv_size : result of converted size of PEM format
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_der_to_pem( void *der, size_t sz_der, size_t * conv_size );
#ifdef __cplusplus
} /* extern "C" */
#endif
