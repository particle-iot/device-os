#ifndef SERVICES2_DYNALIB_H
#define SERVICES2_DYNALIB_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "nanopb_misc.h"
#include <stdint.h>
#ifdef PB_WITHOUT_64BIT
#define pb_int64_t int32_t
#define pb_uint64_t uint32_t
#else
#define pb_int64_t int64_t
#define pb_uint64_t uint64_t
#endif
#endif // defined(DYNALIB_EXPORT)

DYNALIB_BEGIN(services2)

// Nanopb
// Helper functions
DYNALIB_FN(0, services2, pb_ostream_init, pb_ostream_t*(void*))
DYNALIB_FN(1, services2, pb_ostream_free, bool(pb_ostream_t*, void*))
DYNALIB_FN(2, services2, pb_istream_init, pb_istream_t*(void*))
DYNALIB_FN(3, services2, pb_istream_free, bool(pb_istream_t*, void*))
DYNALIB_FN(4, services2, pb_ostream_from_buffer_ex, bool(pb_ostream_t*, pb_byte_t*, size_t, void*))
DYNALIB_FN(5, services2, pb_istream_from_buffer_ex, bool(pb_istream_t*, const pb_byte_t*, size_t, void*))
// Encoding/decoding
DYNALIB_FN(6, services2, pb_encode, bool(pb_ostream_t*, const pb_field_t[], const void*))
DYNALIB_FN(7, services2, pb_get_encoded_size, bool(size_t*, const pb_field_t[], const void*))
DYNALIB_FN(8, services2, pb_encode_tag_for_field, bool(pb_ostream_t*, const pb_field_t*))
DYNALIB_FN(9, services2, pb_encode_submessage, bool(pb_ostream_t*, const pb_field_t[], const void*))
DYNALIB_FN(10, services2, pb_decode_noinit, bool(pb_istream_t*, const pb_field_t[], void*))
DYNALIB_FN(11, services2, pb_read, bool(pb_istream_t*, pb_byte_t*, size_t))
DYNALIB_FN(12, services2, pb_encode_string, bool(pb_ostream_t*, const pb_byte_t*, size_t))
DYNALIB_FN(13, services2, pb_encode_tag, bool(pb_ostream_t*, pb_wire_type_t, uint32_t))
DYNALIB_FN(14, services2, pb_encode_varint, bool(pb_ostream_t*, pb_uint64_t))

DYNALIB_FN(15, services, _printf_float, int(struct _reent*, struct _prt_data_t*, FILE*, int(*pfunc)(struct _reent* , FILE*, const char*, size_t), va_list*))

DYNALIB_END(services2)

#endif  /* SERVICES2_DYNALIB_H */
