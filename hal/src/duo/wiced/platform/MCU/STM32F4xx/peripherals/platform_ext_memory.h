/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include <stdint.h>
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{

    unsigned int    extended_mode : 1;
    /* delay which can be introduced from the end of address set stage, till memory controller */
    /* probes the data bus */
    /* it is given in nanoseconds, the driver will convert it to number of HCLK clock cycles */
    /* the value should lie in the range 0*HCLK up to 15*HCLK */
    unsigned int    data_phase_duration;
    unsigned int    address_set_duration; /* in nanoseconds */
    /* enable burst protocol for read operations */
    /* applicable only for synchronous memories */
    unsigned int    rd_burst : 1;
    /* enable burst protocol for write operations */
    /* applicable only for synchronous memories */
    unsigned int    wr_burst : 1;
    /* enables the NWAIT signal in asynchronous memories */
    unsigned int    async_wait_enabled : 1;
    /* enables the wait enabled signal in synchronous memories */
    unsigned int    sync_wait_enabled : 1;
    /* the polarity of the of the wait signal, 0 - active low, 1 - active high */
    unsigned int    wait_polarity : 1;
    /* specifies the access mode, when extended mode is selected */
    /* a=0, b=1, c=2, or d=3 */
    unsigned int    access_mode : 2;
    /* applicable only for synchronous devices. Number of clocks to issue from the */
    /* moment when address is set till moment when the data is set up  */
    /* in nanoseconds, the memory driver will convert to the value in the range 1*CLK up to 17*CLK */
    unsigned int    data_latency;
    /* output clock frequency, will be used for synchronous memories only */
    /* the memory driver will pick up the proper clock divider using this value */
    /* If the result value is less than that 1/(2*HCLK), it will select the maximum clock freq which is /(2*HCLK) */
    /* It will mean that the memory controller can't support this frequency */
    /* If the result value is more than 1/(17*HCLK), it will return with an error */
    uint32_t        clock_freq;
    /* minimum time between consecutive transactions, this value is in nanoseconds */
    unsigned int    bus_turnaround;
    /* address hold phase duration */
    unsigned int    address_hold_duration;
    /* address set phase duration, applicable only for asynchronous memories */



    /* enable extended mode, when mode and settings for writing transactions are different from modes and */
    /* settings for reading transactions */
    /* and are configured with settings prepended with wr */
    unsigned int    extended_mod : 1;
/* { */
    unsigned int    wr_address_set_duration; /* in nanoseconds */
    unsigned int    wr_address_hold_duration; /* in nanoseconds */
    unsigned int    wr_data_phase_duration; /* in nanoseconds */
    unsigned int    wr_datast; /* in nanoseconds */
    /* minimum time between consecutive writing transactions, this value is in nanoseconds */
    unsigned int    wr_bus_turnaround;
    /* same as access_mode, we are selecting a write mode */
    unsigned int    wr_access_mode: 2;

    /* Number of clocks to issue from the */
    /* moment when address is set till moment when the data is set up  */
    /* in nanoseconds, the memory driver will convert to the value in the range 1*CLK up to 17*CLK */
    unsigned int    wr_data_latency;
/* } */
}stm32f4xx_platform_nor_sram_t;


typedef struct
{
    unsigned int    extended_mode : 1;
}stm32f4xx_platform_nand_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
