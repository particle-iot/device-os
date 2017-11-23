#pragma once

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

pb_ostream_t* pb_ostream_init(void* reserved);
bool pb_ostream_free(pb_ostream_t* stream, void* reserved);

pb_istream_t* pb_istream_init(void* reserved);
bool pb_istream_free(pb_istream_t* stream, void* reserved);

bool pb_ostream_from_buffer_ex(pb_ostream_t* stream, pb_byte_t *buf, size_t bufsize, void* reserved);
bool pb_istream_from_buffer_ex(pb_istream_t* stream, const pb_byte_t *buf, size_t bufsize, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus
