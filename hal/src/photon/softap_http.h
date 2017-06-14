
#pragma once


#if defined(SYSTEM_MINIMAL)
#define SOFTAP_HTTP 0
#else
#define SOFTAP_HTTP 1
#endif


#ifdef __cplusplus
#include <algorithm>

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
                int result = read((uint8_t*)buf, bytes_left);
                buf[len] = 0;
                bytes_left = 0;
                if (result < 0) {
                    // Error
                    free(buf);
                    buf = NULL;
                }
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

        inline void write(const char* s) {
            write((const uint8_t*)s, strlen(s));
        }

        inline void write(uint8_t c) {
        		write(&c, 1);
        }
    };

    struct __attribute__((packed)) Header {
    		uint16_t size;
    		const char* header_list;		// when non-null, a series of headers. Each header MUST be terminated by CRLF.

    		Header(const char* headers) : size(sizeof(*this)), header_list(headers) {}
    };

#else
    typedef void* Reader;
    typedef void* Writer;
    typedef void* Header;
#endif


#ifdef __cplusplus
extern "C" {
#endif

	typedef int (ResponseCallback)(void* cbArg, uint16_t flags, uint16_t responseCode, const char* mimeType, Header* reserved);
    typedef void (PageProvider)(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved);

    int softap_set_application_page_handler(PageProvider* provider, void* reserved);

    PageProvider* softap_get_application_page_handler(void);

#ifdef __cplusplus
}
#endif
