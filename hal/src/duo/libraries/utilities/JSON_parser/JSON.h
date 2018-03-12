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

#include <stdint.h>
#include "wiced_result.h"

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

typedef enum
{
    JSON_STRING_TYPE,
    JSON_NUMBER_TYPE,
    JSON_VALUE_TYPE,
    JSON_ARRAY_TYPE,
    JSON_OBJECT_TYPE,
    JSON_BOOLEAN_TYPE,
    JSON_NULL_TYPE,
    UNKNOWN_JSON_TYPE
} wiced_JSON_types_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
#define OBJECT_START_TOKEN        '{'
#define OBJECT_END_TOKEN          '}'

#define ARRAY_START_TOKEN         '['
#define ARRAY_END_TOKEN           ']'

#define STRING_TOKEN              '"'

#define START_OF_VALUE            ':'

#define COMMA_SEPARATOR           ','

#define ESCAPE_TOKEN              '\\'

#define TRUE_TOKEN                't'

#define FALSE_TOKEN               'f'

#define NULL_TOKEN                'n'
/******************************************************
 *                    Structures
 ******************************************************/
typedef struct json_object{

    char*               object_string;
    uint8_t             object_string_length;
    wiced_JSON_types_t  value_type;
    char*               value;
    uint16_t            value_length;
    struct json_object* parent_object;

} wiced_json_object_t;
/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
typedef wiced_result_t (*wiced_json_callback_t)( wiced_json_object_t* json_object );
/** Register callback to be used by JSON parser
 *
 * @param[in] json_callback                  Callback to be called when JSON parser encounters an objects value. The callback will
 *                                           return the wiced_json_object_type, giving the object string name,value type the object
 *                                           and the value.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_JSON_parser_register_callback( wiced_json_callback_t json_callback );

/** Returns the current callback function registered with by JSON parser
 *
 * @return @ref wiced_json_callback_t
 */
wiced_json_callback_t wiced_JSON_parser_get_callback( void );

/** Parse JSON input string.
 *  This function will parse the JSON input string through a single parse, calling a callback whenever it encounters milestones
 *  an object, passing in object name, json value type, and a value (if value is string or number )
 *
 * @param[in] json_input   : JSON input array

 * @param[in] input_length : Length of JSON input
 *
 * @return @ref wiced_result_t
 *
 * @note: Currently escape values are not supported.
 */
wiced_result_t wiced_JSON_parser( const char* json_input, uint32_t input_length );


#ifdef __cplusplus
} /*extern "C" */
#endif
