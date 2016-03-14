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

#ifdef __cplusplus
    /**
     * Abstraction of an input stream.
     */
    struct __attribute__((packed)) Reader {
        typedef int (*callback_t)(Reader* stream, uint8_t *buf, size_t count);

        callback_t callback;
        size_t bytes_left;
        void* state;

        inline int read(uint8_t* buf, size_t length) {
            int result = -1;
            if (bytes_left) {
                result = callback(this, buf, std::min(length, bytes_left));
                if (result>0)
                    bytes_left -= result;
            }
            return result;
        };

        /**
         * Allocates a buffer with the remaining data as a string.
         * It's the caller's responsibility to free the buffer with free().
         * @return
         */
        char* fetch_as_string() {
            char* buf = (char*)malloc(bytes_left+1);
            if (buf) {
                int len = bytes_left;
                read((uint8_t*)buf, bytes_left);
                buf[len] = 0;
                bytes_left = 0;
            }
            return buf;
        }
    };

    /**
     * Abstraction of an output stream.
     */
    struct __attribute__((packed)) Writer {
        typedef void (*callback_t)(Writer* stream, const uint8_t *buf, size_t count);

        callback_t callback;
        void* state;

        inline void write(const uint8_t* buf, size_t length) {
            callback(this, buf, length);
        }

        void write(const char* s) {
            write((const uint8_t*)s, strlen(s));
        }
    };

#else
    typedef void* Reader;
    typedef void* Writer;

#endif

    typedef int (ResponseCallback)(void* cbArg, uint16_t flags, uint16_t responseCode, const char* mimeType);
    typedef void (PageProvider)(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result);

    int softap_set_application_page_handler(PageProvider* provider, void* reserved);

    PageProvider* softap_get_application_page_handler(void);


#ifdef	__cplusplus
}
#endif

#endif	/* SOFTAP_H */

