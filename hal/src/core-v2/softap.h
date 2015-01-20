/* 
 * File:   softap.h
 * Author: mat
 *
 * Created on 16 December 2014, 08:55
 */

#ifndef SOFTAP_H
#define	SOFTAP_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef void* softap_handle;
    
    struct softap_config {
        void (*softap_complete)();
    };
    
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

    void parse_pubkey_from_privkey(uint8_t* device_pubkey, const uint8_t* device_privkey);

#ifdef	__cplusplus
}
#endif

#endif	/* SOFTAP_H */

