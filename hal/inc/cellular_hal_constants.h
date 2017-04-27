/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef CELLULAR_HAL_CONSTANTS_H
#define CELLULAR_HAL_CONSTANTS_H

#include "inet_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((__packed__))  _CellularConfig_t {
    uint16_t size;
    NetworkConfig nw;
} CellularConfig;

typedef int cellular_result_t;

typedef int (*_CALLBACKPTR_MDM)(int type, const char* buf, int len, void* param);

typedef void (*_CELLULAR_SMS_CB_MDM)(void* data, int index);

#define DEFINE_NET_PROVIDER_DATA \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_TELEFONICA, "spark.telefonica.com", (23*60), (5684) ),  \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_TWILIO, "wireless.twilio.com", (23*60), (4500) ),  \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_MAX, "", (0), (0) )

#define DEFINE_NET_PROVIDER( idx, apn, keepalive, port )  idx
#ifdef __cplusplus
enum CellularNetProv { DEFINE_NET_PROVIDER_DATA };
#else
typedef enum CellularNetProv CellularNetProv;
#endif

#undef DEFINE_NET_PROVIDER
#define DEFINE_NET_PROVIDER( idx, apn, keepalive, port )  { apn, keepalive, port }

#ifdef __cplusplus
struct CellularNetProvData {
    const char* apn;
    int keepalive;
    uint16_t port;
};
#else
typedef struct CellularNetProvData CellularNetProvData;
#endif

#ifdef __cplusplus
// Todo - is storing raw string pointers correct here? These will only be valid
// If they are stored as constants in the application.
struct CellularCredentials
{
    uint16_t size;
    const char* apn = "";
    const char* username = "";
    const char* password = "";

    CellularCredentials()
    {
        size = sizeof(*this);
    }
};
#else
typedef struct CellularCredentials CellularCredentials;
#endif

#ifdef __cplusplus
struct CellularDevice
{
    uint16_t size;
    char iccid[21];
    char imei[16];

    CellularDevice()
    {
        memset(this, 0, sizeof(*this));
        size = sizeof(*this);
    }
};
#else
typedef struct CellularDevice CellularDevice;
#endif

#ifdef __cplusplus
struct CellularSignalHal
{
    int rssi = 0;
    int qual = 0;
};
#else
typedef struct CellularSignalHal CellularSignalHal;
#endif

#ifdef __cplusplus
struct CellularDataHal {
    uint16_t size;
    int cid;
    int tx_session_offset;
    int rx_session_offset;
    int tx_total_offset;
    int rx_total_offset;
    int tx_session;
    int rx_session;
    int tx_total;
    int rx_total;

    CellularDataHal()
    {
        memset(this, 0, sizeof(*this));
        cid = -1;
        size = sizeof(*this);
    }
};
#else
typedef struct CellularDataHal CellularDataHal;
#endif

#ifdef __cplusplus
}
#endif

#endif  /* CELLULAR_HAL_CONSTANTS_H */
