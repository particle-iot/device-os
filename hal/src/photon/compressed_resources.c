#include "wiced_resource.h"
#include "platform_resource.h"
#include "miniz.h"
#include <stddef.h>
#include "debug.h"

__attribute__((weak)) resource_result_t platform_read_external_resource( const resource_hnd_t* resource, uint32_t offset, uint32_t maxsize, uint32_t* size, void* buffer ) {
    static size_t in_pos = 0;
    static tinfl_decompressor* inflator = NULL;
    tinfl_status status = TINFL_STATUS_FAILED;
    resource_result_t res = RESOURCE_FILE_READ_FAIL;

    if (offset == 0) {
        if (inflator == NULL) {
            inflator = (tinfl_decompressor*)malloc(sizeof(tinfl_decompressor));
        }
        tinfl_init(inflator);
        in_pos = 0;
    }

    if (inflator == NULL) {
        *size = 0;
        return res;
    }

    size_t avail_out = maxsize;
    size_t out_pos = 0;

    for(;;) {
        size_t in_bytes = resource->val.comp.size - in_pos;
        size_t out_bytes = avail_out;

        status = tinfl_decompress(inflator, (const mz_uint8*)resource->val.comp.data + in_pos, &in_bytes, (mz_uint8*)buffer, (mz_uint8*)buffer + out_pos, &out_bytes, (in_bytes > 0 ? TINFL_FLAG_HAS_MORE_INPUT : 0) | 0);
        in_pos += in_bytes;
        out_pos += out_bytes;
        avail_out -= out_bytes;

        if ((status <= TINFL_STATUS_DONE) || (!avail_out)) {
            // Output buffer is full or decompression is done
            *size = out_pos;
            res = RESOURCE_SUCCESS;
            if (status != TINFL_STATUS_DONE) {
                return res;
            } else {
                goto cleanup;
            }
        }

        if (status <= TINFL_STATUS_DONE) {
            res = RESOURCE_FILE_READ_FAIL;
            goto cleanup;
        }
    }
cleanup:
    if (inflator != NULL) {
        free(inflator);
        inflator = NULL;
    }
    in_pos = 0;
    return res;
}
