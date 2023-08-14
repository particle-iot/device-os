/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#pragma once

#include <string.h>
#include <stdint.h>

#include "cellular_hal_cellular_global_identity.h"

// ----------------------------------------------------------------
// Types
// ----------------------------------------------------------------
//! MT Device Types
typedef enum {
    DEV_UNKNOWN   = 0,
    DEV_SARA_G350 = 1,
    DEV_LISA_U200 = 2,
    DEV_LISA_C200 = 3,
    DEV_SARA_U260 = 4,
    DEV_SARA_U270 = 5,
    DEV_LEON_G200 = 6,
    DEV_SARA_U201 = 7,
    DEV_SARA_R410 = 8,
    DEV_QUECTEL_BG96 = 9,
    DEV_QUECTEL_EG91_E = 10,
    DEV_QUECTEL_EG91_NA = 11,
    DEV_QUECTEL_EG91_EX = 12,
    DEV_SARA_R510 = 13,
    DEV_QUECTEL_BG95_M1 = 14,
    DEV_QUECTEL_EG91_NAX = 15,
    DEV_QUECTEL_BG77 = 16,
    DEV_QUECTEL_BG95_MF = 17,
    DEV_QUECTEL_BG95_M6 = 18,
    DEV_QUECTEL_BG95_M5 = 19,

} Dev;
//! SIM Status
typedef enum {
    SIM_UNKNOWN = 0,
    SIM_MISSING = 1,
    SIM_PIN     = 2,
    SIM_READY   = 3
} Sim;
//! SIM Status
typedef enum {
    LPM_DISABLED = 0,
    LPM_ENABLED  = 1,
    LPM_ACTIVE   = 2
} Lpm;
//! Device status
typedef struct {
    Dev dev;            //!< Device Type
    Lpm lpm;            //!< Power Saving
    Sim sim;            //!< SIM Card Status
    char ccid[20+1];    //!< Integrated Circuit Card ID
    char imsi[15+1];    //!< International Mobile Station Identity
    char imei[15+1];    //!< International Mobile Equipment Identity
    char meid[18+1];    //!< Mobile Equipment IDentifier
    char manu[16];      //!< Manufacturer (u-blox)
    char model[16];     //!< Model Name (LISA-U200, LISA-C200 or SARA-G350)
    char ver[16+1];       //!< Software Version
} DevStatus;
//! Registration Status
// Registration Status is now handled with CellularRegistrationStatus()
//! Access Technology
typedef enum {
    ACT_UNKNOWN     = 0,
    ACT_GSM         = 1,
    ACT_EDGE        = 2,
    ACT_UTRAN       = 3,
    ACT_CDMA        = 4,
    ACT_LTE         = 5,
    ACT_LTE_CAT_M1  = 6,
    ACT_LTE_CAT_NB1 = 7
} AcT;
//! Ublox-specific RAT
typedef enum {
    UBLOX_SARA_RAT_NONE              = -1,
    UBLOX_SARA_RAT_GSM               = 0,
    UBLOX_SARA_RAT_GSM_COMPACT       = 1,
    UBLOX_SARA_RAT_UTRAN             = 2,
    UBLOX_SARA_RAT_GSM_EDGE          = 3,
    UBLOX_SARA_RAT_UTRAN_HSDPA       = 4,
    UBLOX_SARA_RAT_UTRAN_HSUPA       = 5,
    UBLOX_SARA_RAT_UTRAN_HSDPA_HSUPA = 6,
    UBLOX_SARA_RAT_LTE               = 7,
    UBLOX_SARA_RAT_LTE_CAT_M1        = 8,
    UBLOX_SARA_RAT_LTE_NB_IOT        = 9
} UbloxSaraCellularAccessTechnology;
//! Ublox UMNOPROF settings
typedef enum {
    UBLOX_SARA_UMNOPROF_NONE             = -1,
    UBLOX_SARA_UMNOPROF_SW_DEFAULT       = 0,
    UBLOX_SARA_UMNOPROF_SIM_SELECT       = 1,
    UBLOX_SARA_UMNOPROF_ATT              = 2,
    UBLOX_SARA_UMNOPROF_VERIZON          = 3,
    UBLOX_SARA_UMNOPROF_TELSTRA          = 4,
    UBLOX_SARA_UMNOPROF_TMOBILE          = 5,
    UBLOX_SARA_UMNOPROF_CHINA_TELECOM    = 6,
    UBLOX_SARA_UMNOPROF_SPRINT           = 8,
    UBLOX_SARA_UMNOPROF_VODAFONE         = 19,
    UBLOX_SARA_UMNOPROF_TELUS            = 21,
    UBLOX_SARA_UMNOPROF_DEUTSCHE_TELEKOM = 31,
    UBLOX_SARA_UMNOPROF_GLOBAL           = 90,
    UBLOX_SARA_UMNOPROF_STANDARD_EUROPE  = 100,
} UbloxSaraUmnoprof;

typedef enum UbloxSaraUsoer {
    UBLOX_SARA_USOER_UNKNOWN = -1,
    UBLOX_SARA_USOER_NONE = 0,
    UBLOX_SARA_USOER_EPERM = 1,
    UBLOX_SARA_USOER_ENOENT = 2,
    UBLOX_SARA_USOER_EINTR = 4,
    UBLOX_SARA_USOER_EIO = 5,
    UBLOX_SARA_USOER_EBADF = 9,
    UBLOX_SARA_USOER_ECHILD = 10,
    UBLOX_SARA_USOER_EWOULDBLOCK = 11,
    UBLOX_SARA_USOER_ENOMEM = 12,
    UBLOX_SARA_USOER_EFAULT = 14,
    UBLOX_SARA_USOER_EINVAL = 22,
    UBLOX_SARA_USOER_EPIPE = 32,
    UBLOX_SARA_USOER_ENOSYS = 38,
    UBLOX_SARA_USOER_ENONET = 64,
    UBLOX_SARA_USOER_EEOF = 65,
    UBLOX_SARA_USOER_EPROTO = 71,
    UBLOX_SARA_USOER_EBADFD = 77,
    UBLOX_SARA_USOER_EREMCHG = 78,
    UBLOX_SARA_USOER_EDESTADDRREQ = 89,
    UBLOX_SARA_USOER_EPROTOTYPE = 91,
    UBLOX_SARA_USOER_ENOPROTOOPT = 92,
    UBLOX_SARA_USOER_EPROTONOSUPPORT = 93,
    UBLOX_SARA_USOER_ESOCKTNNOSUPPORT = 94,
    UBLOX_SARA_USOER_EOPNOTSUPP = 95,
    UBLOX_SARA_USOER_EPFNOSUPPORT = 96,
    UBLOX_SARA_USOER_EAFNOSUPPORT = 97,
    UBLOX_SARA_USOER_EADDRINUSE = 98,
    UBLOX_SARA_USOER_EADDRNOTAVAIL = 99,
    UBLOX_SARA_USOER_ENETDOWN = 100,
    UBLOX_SARA_USOER_ENETUNREACH = 101,
    UBLOX_SARA_USOER_ENETRESET = 102,
    UBLOX_SARA_USOER_ECONNABORTED = 103,
    UBLOX_SARA_USOER_ECONNRESET = 104,
    UBLOX_SARA_USOER_ENOBUFS = 105,
    UBLOX_SARA_USOER_EISCONN = 106,
    UBLOX_SARA_USOER_ENOTCONN = 107,
    UBLOX_SARA_USOER_ESHUTDOWN = 108,
    UBLOX_SARA_USOER_ETIMEDOUT = 110,
    UBLOX_SARA_USOER_ECONNREFUSED = 111,
    UBLOX_SARA_USOER_EHOSTDOWN = 112,
    UBLOX_SARA_USOER_EHOSTUNREACH = 113,
    UBLOX_SARA_USOER_EINPROGRESS = 115,
    UBLOX_SARA_USOER_ENSRNODATA = 160,
    UBLOX_SARA_USOER_ENSRFORMERR = 161,
    UBLOX_SARA_USOER_ENSRSERVFAIL = 162,
    UBLOX_SARA_USOER_ENSRNOTFOUND = 163,
    UBLOX_SARA_USOER_ENSRNOTIMP = 164,
    UBLOX_SARA_USOER_ENSRREFUSED = 165,
    UBLOX_SARA_USOER_ENSRBADQUERY = 166,
    UBLOX_SARA_USOER_ENSRBADNAME = 167,
    UBLOX_SARA_USOER_ENSRBADFAMILY = 168,
    UBLOX_SARA_USOER_ENSRBADRESP = 169,
    UBLOX_SARA_USOER_ENSRCONNREFUSED = 170,
    UBLOX_SARA_USOER_ENSRTIMEOUT = 171,
    UBLOX_SARA_USOER_ENSROF = 172,
    UBLOX_SARA_USOER_ENSRFILE = 173,
    UBLOX_SARA_USOER_ENSRNOMEM = 174,
    UBLOX_SARA_USOER_ENSRDESTRUCTION = 175,
    UBLOX_SARA_USOER_ENSRQUERYDOMAINTOOLONG = 176,
    UBLOX_SARA_USOER_ENSRCNAMELOOP = 177,
} UbloxSaraUsoer;

//! Network Status
typedef struct {
    AcT act;        //!< Access Technology

    union {
        int rxlev;  //!< GSM RAT: RXLEV ([0, 63], 99), see 3GPP TS 45.008 subclause 8.1.4
        int rscp;   //!< UMTS RAT: RSCP ([-5, 91], 255), see 3GPP TS 25.133 subclause 9.1.1.3
        int rsrp;   //!< LTE RAT: RSRP ([0, 95], 255), see 3GPP TS 36.133 subclause 9.1.4
        int asu;    //!< Abstract accessor
    };

    union {
        int rxqual; //!< GSM RAT: RXQUAL ([0, 7], 99), see 3GPP TS 45.008 subclause 8.2.4
        int ecno;   //!< UMTS RAT: ECNO ([0, 49], 255), see 3GPP TS 25.133 subclause 9.1.2.3
        int rsrq;   //!< LTE RAT: RSRQ ([0, 97], 255), see 3GPP TS 36.133 subclause 9.1.7
        int aqual;  //!< Abstract accessor
    };

    CellularGlobalIdentity cgi;  //!< Cellular Global Identity (MCC, MNC, LAC, CI)
    char num[32];   //!< Mobile Directory Number
    int cops;       //!< COPS mode
} NetStatus;

#ifdef __cplusplus
//! Data Usage struct
struct MDM_DataUsage {
    uint16_t size;
    int cid;
    int tx_session;
    int rx_session;
    int tx_total;
    int rx_total;

    MDM_DataUsage()
    {
        memset(this, 0, sizeof(*this));
        size = sizeof(*this);
    }
};
#else
typedef struct MDM_DataUsage MDM_DataUsage;
#endif

//! Bands
// NOTE: KEEP IN SYNC with band_enums[] array in spark_wiring_cellular_printable.h
typedef enum { BAND_DEFAULT=0, BAND_0=0, BAND_700=700, BAND_800=800, BAND_850=850,
               BAND_900=900, BAND_1500=1500, BAND_1700=1700, BAND_1800=1800,
               BAND_1900=1900, BAND_2100=2100, BAND_2600=2600 } MDM_Band;

#ifdef __cplusplus
//! Band Select struct
struct MDM_BandSelect {
    uint16_t size;
    int count;
    MDM_Band band[5];

    MDM_BandSelect()
    {
        memset(this, 0, sizeof(*this));
        size = sizeof(*this);
    }
};
#else
typedef struct MDM_BandSelect MDM_BandSelect;
#endif

//! An IP v4 address
typedef uint32_t MDM_IP;
#define NOIP ((MDM_IP)0) //!< No IP address
// IP number formating and conversion
#define IPSTR           "%d.%d.%d.%d"
#define IPNUM(ip)       (int)(((ip)>>24)&0xff), \
                        (int)(((ip)>>16)&0xff), \
                        (int)(((ip)>> 8)&0xff), \
                        (int)(((ip)>> 0)&0xff)
#define IPADR(a,b,c,d) ((((uint32_t)(a))<<24) | \
                        (((uint32_t)(b))<<16) | \
                        (((uint32_t)(c))<< 8) | \
                        (((uint32_t)(d))<< 0))


// ----------------------------------------------------------------
// Device
// ----------------------------------------------------------------

typedef enum {
    AUTH_NONE   = 0,
    AUTH_PAP    = 1,
    AUTH_CHAP   = 2,
    AUTH_DETECT = 3
} Auth;

// ----------------------------------------------------------------
// Sockets
// ----------------------------------------------------------------

//! Type of IP protocol
typedef enum {
    MDM_IPPROTO_TCP = 0,
    MDM_IPPROTO_UDP = 1
} IpProtocol;

//! Socket error return codes
#define MDM_SOCKET_ERROR    (-1)

// ----------------------------------------------------------------
// Parsing
// ----------------------------------------------------------------

enum {
    // waitFinalResp Responses
    NOT_FOUND     =  0,
    WAIT          = -1, // TIMEOUT
    RESP_OK       = -2,
    RESP_ERROR    = -3,
    RESP_PROMPT   = -4,
    RESP_ABORTED  = -5,

    // getLine Responses
    #define LENGTH(x)  (x & 0x00FFFF) //!< extract/mask the length
    #define TYPE(x)    (x & 0xFF0000) //!< extract/mask the type

    TYPE_UNKNOWN    = 0x000000,
    TYPE_OK         = 0x110000,
    TYPE_ERROR      = 0x120000,
    TYPE_RING       = 0x210000,
    TYPE_CONNECT    = 0x220000,
    TYPE_NOCARRIER  = 0x230000,
    TYPE_NODIALTONE = 0x240000,
    TYPE_BUSY       = 0x250000,
    TYPE_NOANSWER   = 0x260000,
    TYPE_PROMPT     = 0x300000,
    TYPE_PLUS       = 0x400000,
    TYPE_USORF_1    = 0x410000,
    TYPE_TEXT       = 0x500000,
    TYPE_ABORTED    = 0x600000,
    TYPE_DBLNEWLINE = 0x700000,

    // special timout constant
    TIMEOUT_BLOCKING = 0xffffffff
};
