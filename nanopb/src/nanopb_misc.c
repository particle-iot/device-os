#include <stdlib.h>
#include "nanopb_misc.h"

pb_ostream_t* pb_ostream_init(void* reserved) {
    return (pb_ostream_t*)calloc(sizeof(pb_ostream_t), 1);
}

bool pb_ostream_free(pb_ostream_t* stream, void* reserved) {
    if (stream != NULL) {
        free(stream);
        return true;
    }

    return false;
}

pb_istream_t* pb_istream_init(void* reserved) {
    return (pb_istream_t*)calloc(sizeof(pb_istream_t), 1);
}

bool pb_istream_free(pb_istream_t* stream, void* reserved) {
    if (stream != NULL) {
        free(stream);
        return true;
    }

    return false;
}

bool pb_ostream_from_buffer_ex(pb_ostream_t* stream, pb_byte_t *buf, size_t bufsize, void* reserved) {
    if (stream != NULL) {
        pb_ostream_t tmp = pb_ostream_from_buffer(buf, bufsize);
        memcpy(stream, &tmp, sizeof(pb_ostream_t));
        return true;
    }

    return false;
}

bool pb_istream_from_buffer_ex(pb_istream_t* stream, const pb_byte_t *buf, size_t bufsize, void* reserved) {
    if (stream != NULL) {
        pb_istream_t tmp = pb_istream_from_buffer(buf, bufsize);
        memcpy(stream, &tmp, sizeof(pb_istream_t));
        return true;
    }

    return false;
}
