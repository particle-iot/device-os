/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef TRACE_ACTION_H_
#define TRACE_ACTION_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * This enumerator describes all of the possible scheduler actions that we may
 * want to trace.
 *
 * @remarks When writing trace actions to the buffer, it is assumed that this
 * enum will fit into 4-bits.
 */
typedef enum
{
#define TRACE_ACTION_T_START  0x0
    Trace_Invalid           = 0x0,
    Trace_Create            = 0x1,
    Trace_Delete            = 0x2,
    Trace_Suspend           = 0x3,
    Trace_Resume            = 0x4,
    Trace_ResumeFromISR     = 0x5,
    Trace_Delay             = 0x6,
    Trace_Die               = 0x7,
    Trace_PrioritySet       = 0x8,
    Trace_SwitchOut         = 0x9,
    Trace_SwitchIn          = 0xA,
    Trace_Executing         = 0xB
#define TRACE_ACTION_T_END    0xB
#define TRACE_ACTION_T_MASK   0xF
} trace_action_t;

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* TRACE_ACTION_H_ */
