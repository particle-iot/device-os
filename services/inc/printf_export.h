#pragma once

#if PLATFORM_ID != 3 && PLATFORM_ID != 20

#include <stdarg.h>
#include <stdio.h>

// Taken from nano-vfprintf_local.h

/* BUF must be big enough for the maximum %#llo (assuming long long is
   at most 64 bits, this would be 23 characters), the maximum
   multibyte character %C, and the maximum default precision of %La
   (assuming long double is at most 128 bits with 113 bits of
   mantissa, this would be 29 characters).  %e, %f, and %g use
   reentrant storage shared with mprec.  All other formats that use
   buf get by with fewer characters.  Making BUF slightly bigger
   reduces the need for malloc in %.*a and %S, when large precision or
   long strings are processed.
   The bigger size of 100 bytes is used on systems which allow number
   strings using the locale's grouping character.  Since that's a multibyte
   value, we should use a conservative value.  */
#define BUF     40

/* All data needed to decode format string are kept in below struct.  */
struct _prt_data_t
{
  int flags;        /* Flags.  */
  int prec;     /* Precision.  */
  int dprec;        /* Decimal precision.  */
  int width;        /* Width.  */
  int size;     /* Size of converted field or string.  */
  int ret;      /* Return value accumulator.  */
  char code;        /* Current conversion specifier.  */
  char blank;       /* Blank character.  */
  char zero;        /* Zero character.  */
  char buf[BUF];    /* Output buffer for non-floating point number.  */
  char l_buf[3];    /* Sign&hex_prefix, "+/-" and "0x/X".  */
#ifdef FLOATING_POINT
  _PRINTF_FLOAT_TYPE _double_;  /* Double value.  */
  char expstr[MAXEXPLEN];   /* Buffer for exponent string.  */
  int lead;     /* The sig figs before decimal or group sep.  */
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

int
_printf_float (struct _reent *data,
           struct _prt_data_t *pdata,
           FILE *fp,
           int (*pfunc)(struct _reent *, FILE *,
                const char *, size_t len),
           va_list *ap);

int
_printf_i (struct _reent *data, struct _prt_data_t *pdata, FILE *fp,
     int (*pfunc)(struct _reent *, FILE *, const char *, size_t len),
     va_list *ap);

int _svfprintf_r(struct _reent *, FILE *, const char *, va_list) _ATTRIBUTE ((__format__ (__printf__, 3, 0)));

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_ID != 3 && PLATFORM_ID != 20
