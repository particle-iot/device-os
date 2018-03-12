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
 *  Definition for the FreeRTOS system tick frequency
 *  Avoids a include file loop
 *
 */

#ifndef INCLUDED_WWD_FREERTOS_SYSTICK_H_
#define INCLUDED_WWD_FREERTOS_SYSTICK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define SYSTICK_FREQUENCY  (1000)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_FREERTOS_SYSTICK_H_ */
