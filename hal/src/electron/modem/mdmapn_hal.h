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

#include "stm32f2xx.h"

#ifndef NULL
    #define NULL ((void *)0)
#endif

/* ----------------------------------------------------------------
   APN stands for Access Point Name, a setting on your modem or phone
   that identifies an external network your phone can access for data
   (e.g. 3G or 4G Internet service on your phone).

   The APN settings can be forced when calling the join function.
   Below is a list of known APNs that us used if no apn config
   is forced. This list could be extended by other settings.

   For further reading:
   wiki apn: http://en.wikipedia.org/wiki/Access_Point_Name
   wiki mcc/mnc: http://en.wikipedia.org/wiki/Mobile_country_code
   google: https://www.google.de/search?q=APN+list
---------------------------------------------------------------- */

//! helper to generate the APN string
#define _APN(apn,username,password) apn "\0" username "\0" password "\0"

//! helper to extract a field from the config string
#define _APN_GET(cfg) \
    *cfg ? cfg : ""; \
    cfg  += strlen(cfg) + 1

//! APN lookup struct
typedef struct {
    const char* mccmnc; //!< mobile country code (MCC) and mobile network code MNC
    const char* cfg;    //!< APN configuartion string, use _APN macro to generate
} APN_t;

//! default APN settings used by many networks
static const char* apndef = _APN(,,)
                            _APN("internet",,);

/*! this is a list of special APNs for different network operators
    There is no need to enter the default apn internet in the table;
    apndef will be used if no entry matches.

    The APN without username/password have to be listed first.
*/
static const APN_t apnlut[] = {
// MCC Country
//  { /* Operator */ "MCC-MNC[,MNC]" _APN(APN,USERNAME,PASSWORD) },
// MCC must be 3 digits
// MNC must be either 2 or 3 digits
// MCC must be separated by '-' from MNC, multiple MNC can be separated by ','

// 232 Austria - AUT
    { /* T-Mobile */ "232-03",  _APN("m2m.business",,) },

// 460 China - CN
    { /* CN Mobile */"460-00",  _APN("cmnet",,)
                                _APN("cmwap",,) },
    { /* Unicom */   "460-01",  _APN("3gnet",,)
                                _APN("uninet","uninet","uninet") },

// 262 Germany - DE
    { /* T-Mobile */ "262-01",  _APN("internet.t-mobile","t-mobile","tm") },
    { /* T-Mobile */ "262-02,06",
                                _APN("m2m.business",,) },

// 222 Italy - IT
    { /* TIM */      "222-01",  _APN("ibox.tim.it",,) },
    { /* Vodafone */ "222-10",  _APN("web.omnitel.it",,) },
    { /* Wind */     "222-88",  _APN("internet.wind.biz",,) },

// 440 Japan - JP
    { /* Softbank */ "440-04,06,20,40,41,42,43,44,45,46,47,48,90,91,92,93,94,95"
                         ",96,97,98"
                                _APN("open.softbank.ne.jp","opensoftbank","ebMNuX1FIHg9d3DA")
                                _APN("smile.world","dna1trop","so2t3k3m2a") },
    { /* NTTDoCoMo */"440-09,10,11,12,13,14,15,16,17,18,19,21,22,23,24,25,26,27,"
                         "28,29,30,31,32,33,34,35,36,37,38,39,58,59,60,61,62,63,"
                         "64,65,66,67,68,69,87,99",
                                _APN("bmobilewap",,) /*BMobile*/
                                _APN("mpr2.bizho.net","Mopera U",) /* DoCoMo */
                                _APN("bmobile.ne.jp","bmobile@wifi2","bmobile") /*BMobile*/ },

// 204 Netherlands - NL
    { /* Vodafone */ "204-04",  _APN("public4.m2minternet.com",,) },

// 293 Slovenia - SI
    { /* Si.mobil */ "293-40",  _APN("internet.simobil.si",,) },
    { /* Tusmobil */ "293-70",  _APN("internet.tusmobil.si",,) },

// 240 Sweden SE
    { /* Telia */    "240-01",  _APN("online.telia.se",,) },
    { /* Telenor */  "240-06,08",
                                _APN("services.telenor.se",,) },
    { /* Tele2 */    "240-07",  _APN("mobileinternet.tele2.se",,) },

// 228 Switzerland - CH
    { /* Swisscom */ "228-01",  _APN("gprs.swisscom.ch",,) },
    { /* Orange */   "228-03",  _APN("internet",,) /* contract */
                                _APN("click",,)    /* pre-pay */ },

// 234 United Kingdom - GB
    { /* O2 */       "234-02,10,11",
                                _APN("mobile.o2.co.uk","faster","web") /* contract */
                                _APN("mobile.o2.co.uk","bypass","web") /* pre-pay */
                                _APN("payandgo.o2.co.uk","payandgo","payandgo") },
    { /* Vodafone */ "234-15",  _APN("internet","web","web")          /* contract */
                                _APN("pp.vodafone.co.uk","wap","wap")  /* pre-pay */ },
    { /* Three */    "234-20",  _APN("three.co.uk",,) },

// 310 United States of America - US
    { /* T-Mobile */ "310-026,260,490",
                                _APN("epc.tmobile.com",,)
                                _APN("fast.tmobile.com",,) /* LTE */ },
    { /* AT&T */     "310-030,150,170,260,410,560,680",
                                _APN("phone",,)
                                _APN("wap.cingular","WAP@CINGULARGPRS.COM","CINGULAR1")
                                _APN("isp.cingular","ISP@CINGULARGPRS.COM","CINGULAR1") },

// 901 International - INT
    { /* Transatel */ "901-37", _APN("netgprs.com","tsl","tsl") },
};

inline const char* apnconfig(const char* imsi)
{
    const char* config = NULL;
    if (imsi && *imsi) {
        // many carriers use internet without username and password, os use this as default
        // now try to lookup the setting for our table
        for (uint32_t i = 0; i < sizeof(apnlut)/sizeof(*apnlut) && !config; i ++) {
            const char* p = apnlut[i].mccmnc;
            // check the MCC
            if ((0 == memcmp(imsi, p, 3))) {
                p += 3;
                // check all the MNC, MNC length can be 2 or 3 digits
                while (((p[0] == '-') || (p[0] == ',')) &&
                        (p[1] >= '0') && (p[1] <= '9') &&
                        (p[2] >= '0') && (p[2] <= '9') && !config) {
                    int l = ((p[3] >= '0') && (p[3] <= '9')) ? 3 : 2;
                    if (0 == memcmp(imsi+3,p+1,l))
                        config = apnlut[i].cfg;
                    p += 1 + l;
                }
            }
        }
    }
    // use default if not found
    if (!config)
        config = apndef;
    return config;
}
