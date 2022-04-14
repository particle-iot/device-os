#ifndef SERVICES2_DYNALIB_H
#define SERVICES2_DYNALIB_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "nanopb_misc.h"
#include "printf_export.h"
#include <stdint.h>
#include <time.h>
#include "time_compat.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void _tzset_unlocked_r(struct _reent*);
unsigned long __udivmoddi4 (unsigned long a, unsigned long b, unsigned long *c);
int __ssvfscanf_r(struct _reent *r, FILE *f, const char *fmt, va_list va);

#ifdef __cplusplus
}
#endif // __cplusplus

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

DYNALIB_FN(15, services2, _printf_float, int(struct _reent*, struct _prt_data_t*, FILE*, int(*pfunc)(struct _reent* , FILE*, const char*, size_t), va_list*))
DYNALIB_FN(16, services2, _tzset_unlocked_r, void(struct _reent*))
// FIXME: this doesn't work with LTO, disable for now when importing
#if !(defined(DYNALIB_IMPORT) && (defined(PARTICLE_COMPILE_LTO) || defined(PARTICLE_COMPILE_LTO_FAT)))
DYNALIB_FN(17, services2, __udivmoddi4, unsigned long(unsigned long, unsigned long, unsigned long*))
#else
DYNALIB_FN_PLACEHOLDER(17, services2)
#endif // !(defined(DYNALIB_IMPORT) && (defined(PARTICLE_COMPILE_LTO) || defined(PARTICLE_COMPILE_LTO_FAT)))
DYNALIB_FN(18, services2, mktime32, time32_t(struct tm*))
DYNALIB_FN(19, services2, __ssvfscanf_r, int(struct _reent*, FILE*, const char*, va_list))
DYNALIB_FN(20, services2, _printf_i, int(struct _reent*, struct _prt_data_t*, FILE*, int (*pfunc)(struct _reent *, FILE *, const char *, size_t), va_list*))
DYNALIB_FN(21, services2, localtime32_r, struct tm*(const time32_t*, struct tm*))
DYNALIB_FN(22, services2, localtime_r, struct tm*(const time_t*, struct tm*))
DYNALIB_FN(23, services2, mktime, time_t(struct tm*))
DYNALIB_FN(24, services2, gmtime_r, struct tm*(const time_t*, struct tm*))
DYNALIB_FN(25, services2, strftime, size_t(char* __restrict, size_t, const char* __restrict, const struct tm* __restrict))
// FIXME: this is a strong symbol in newer versions of dynalib avoid importing
#ifndef DYNALIB_IMPORT
DYNALIB_FN(26, services2, _svfprintf_r, int(struct _reent*, FILE*, const char*, va_list))
#else
DYNALIB_FN_PLACEHOLDER(26, services2)
#endif // DYNALIB_IMPORT

DYNALIB_END(services2)

#endif  /* SERVICES2_DYNALIB_H */
