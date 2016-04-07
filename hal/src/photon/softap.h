/*
 * File:   softap.h
 * Author: mat
 *
 * Created on 16 December 2014, 08:55
 */

#ifndef SOFTAP_H
#define	SOFTAP_H

#include "wiced.h"          // for wiced_result_t

#ifdef	__cplusplus
#include <algorithm>
extern "C" {
#endif

    typedef void* softap_handle;

    typedef struct softap_config {
        void (*softap_complete)();
    } softap_config;

    /**
     * Starts the soft ap setup process.
     * @param config The soft ap configuration details.
     * @return The softap handle, or NULL if soft ap could not be started.
     *
     * The softap config runs asynchronously.
     */
    softap_handle softap_start(softap_config* config);

    void softap_stop(void* pv);

    wiced_result_t add_wiced_wifi_credentials(const char *ssid, uint16_t ssidLen, const char *password,
        uint16_t passwordLen, wiced_security_t security, unsigned channel);

    size_t hex_decode(uint8_t* buf, size_t len, const char* hex);
    uint8_t hex_nibble(unsigned char c);

#ifdef	__cplusplus
}
#endif

#endif	/* SOFTAP_H */

