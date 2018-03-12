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


/** List of Base64 conversion standards
 *
 */
typedef enum
{
    BASE64_STANDARD                         = ( ( '+' << 16 ) | ( '/' << 8 ) | '=' ),  /* RFC 1421, 2045, 3548, 4648, 4880 */
    BASE64_NO_PADDING                       = ( ( '+' << 16 ) | ( '/' << 8 )       ),  /* RFC 1642, 3548, 4648 */
    BASE64_URL_SAFE_CHARSET                 = ( ( '-' << 16 ) | ( '_' << 8 )       ),  /* RFC 4648 */
    BASE64_URL_SAFE_CHARSET_WITH_PADDING    = ( ( '-' << 16 ) | ( '_' << 8 ) | '=' ),  /* RFC 4648 */
    BASE64_Y64                              = ( ( '.' << 16 ) | ( '_' << 8 ) | '-' ),
    BASE64_XML_TOKEN                        = ( ( '.' << 16 ) | ( '-' << 8 )       ),
    BASE64_XML_IDENTIFIER                   = ( ( '_' << 16 ) | ( ':' << 8 )       ),
    BASE64_PROG_IDENTIFIER1                 = ( ( '_' << 16 ) | ( '-' << 8 )       ),
    BASE64_PROG_IDENTIFIER2                 = ( ( '.' << 16 ) | ( '_' << 8 )       ),
    BASE64_REGEX                            = ( ( '!' << 16 ) | ( '-' << 8 )       ),
} base64_options_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


/** Returns true if character whitespace
 *
 * I.e. if character is a Tab, Line-Feed, Vertical Tab, Form Feed, Carriage-Return or a Space
 *
 * @param[in] c : The character to be tested
 *
 * @return true if character whitespace
 */
int isspace( int c );

/** Encodes data into Base-64 coding which can be sent safely as text
 *
 * Terminating null will be appended.
 *
 * @param[in] src         : A pointer to the source data to be converted
 * @param[in] src_length  : The length of data to be converted (or -1 if the data is a null terminated string
 * @param[out] target     : The buffer that will receive the encoded data. NOTE: src and target can't be pointing to the same buffer.
 * @param[in] target_size : The size of the output buffer in bytes - will need to be at least 4*(src_length+2)/3
 * @param[in] options     : Specifies which Base64 encoding standard to use - see @ref base64_options_t
 *
 * @return number of Base64 characters output (not including terminating null),  otherwise negative indicates an error
 */
int base64_encode( unsigned char const* src, int32_t src_length, unsigned char* target, uint32_t target_size, base64_options_t options );

/** Decodes data from Base-64 coding which can be sent safely as text
 *
 * Terminating null will be appended.
 *
 * @param[in] src         : A pointer to the source Base64 coded data to be decoded
 * @param[in] src_length  : The length of data to be converted (or -1 if the data is a null terminated string
 * @param[out] target     : The buffer that will receive the decoded data.
 * @param[in] target_size : The size of the output buffer in bytes - will need to be at least 3*(src_length+3)/4
 * @param[in] options     : Specifies which Base64 encoding standard to use - see @ref base64_options_t
 *
 * @return number of decoded characters output (not including terminating null),  otherwise negative indicates an error
 */
int base64_decode( unsigned char const* src, int32_t src_length, unsigned char* target, uint32_t target_size, base64_options_t options );

#ifdef __cplusplus
} /* extern "C" */
#endif
