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

#include "wiced_result.h"
#include "wiced_utilities.h"

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

typedef struct linked_list_node linked_list_node_t;

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
struct linked_list_node
{
    void*               data;
    linked_list_node_t* next;
    linked_list_node_t* prev;
};

typedef struct
{
    uint32_t            count;
    linked_list_node_t* front;
    linked_list_node_t* rear;
} linked_list_t;
#pragma pack()

typedef wiced_bool_t (*linked_list_compare_callback_t)( linked_list_node_t* node_to_compare, void* user_data );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t linked_list_init( linked_list_t* list );

wiced_result_t linked_list_deinit( linked_list_t* list );

wiced_result_t linked_list_get_count( linked_list_t* list, uint32_t* count );

wiced_result_t linked_list_set_node_data( linked_list_node_t* node, const void* data );

wiced_result_t linked_list_get_front_node( linked_list_t* list, linked_list_node_t** front_node );

wiced_result_t linked_list_get_rear_node( linked_list_t* list, linked_list_node_t** rear_node );

wiced_result_t linked_list_find_node( linked_list_t* list, linked_list_compare_callback_t callback, void* user_data, linked_list_node_t** node_found );

wiced_result_t linked_list_insert_node_at_front( linked_list_t* list, linked_list_node_t* node );

wiced_result_t linked_list_insert_node_at_rear( linked_list_t* list, linked_list_node_t* node );

wiced_result_t linked_list_insert_node_before( linked_list_t* list, linked_list_node_t* reference_node, linked_list_node_t* node_to_insert );

wiced_result_t linked_list_insert_node_after( linked_list_t* list, linked_list_node_t* reference_node, linked_list_node_t* node_to_insert );

wiced_result_t linked_list_remove_node( linked_list_t* list, linked_list_node_t* node );

wiced_result_t linked_list_remove_node_from_front( linked_list_t* list, linked_list_node_t** removed_node );

wiced_result_t linked_list_remove_node_from_rear( linked_list_t* list, linked_list_node_t** removed_node );

#ifdef __cplusplus
} /* extern "C" */
#endif
