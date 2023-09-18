/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "platforms.h"

#if PLATFORM_ID != PLATFORM_GCC

#define __MISC_VISIBLE (1)
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

// Copy of nano-vfscanf.c, nano-vfscanf_i.c nano-vfscanf_local.h
// Original copyright notices:

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 2012-2014 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define STRING_ONLY

#ifndef NO_FLOATING_POINT
#define FLOATING_POINT
#endif

extern int
_sungetc_r (struct _reent *data,
	int c,
	register FILE *fp);

extern int
__ssrefill_r (struct _reent * ptr,
       register FILE * fp);

#ifdef STRING_ONLY
#undef _newlib_flockfile_start
#undef _newlib_flockfile_exit
#undef _newlib_flockfile_end
#define _newlib_flockfile_start(x) {}
#define _newlib_flockfile_exit(x) {}
#define _newlib_flockfile_end(x) {}
#define _ungetc_r _sungetc_r
#define __srefill_r __ssrefill_r
#endif

#ifdef FLOATING_POINT
#include <math.h>
#include <float.h>

/* Currently a test is made to see if long double processing is warranted.
   This could be changed in the future should the _ldtoa_r code be
   preferred over _dtoa_r.  */
#define _NO_LONGDBL

#ifdef _NO_LONGDBL
/* 11-bit exponent (VAX G floating point) is 308 decimal digits */
#define	MAXEXP		308
#else  /* !_NO_LONGDBL */
/* 15-bit exponent (Intel extended floating point) is 4932 decimal digits */
#define MAXEXP          4932
#endif /* !_NO_LONGDBL */
/* 128 bit fraction takes up 39 decimal digits; max reasonable precision */
#define	MAXFRACT	39

#if ((MAXEXP+MAXFRACT+3) > MB_LEN_MAX)
/* "3 = sign + decimal point + NUL".  */
# define BUF (MAXEXP+MAXFRACT+3)
#else
# define BUF MB_LEN_MAX
#endif

/* An upper bound for how long a long prints in decimal.  4 / 13 approximates
   log (2).  Add one char for roundoff compensation and one for the sign.  */
#define MAX_LONG_LEN ((CHAR_BIT * sizeof (long)  - 1) * 4 / 13 + 2)
#else
#define	BUF	40
#endif


#define _NO_LONGLONG
#undef _WANT_IO_C99_FORMATS
#undef _WANT_IO_POS_ARGS

#define _NO_POS_ARGS

/* Macros for converting digits to letters and vice versa.  */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit (c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * Flags used during conversion.
 */

#define	SHORT		0x01	/* "h": short.  */
#define	LONG		0x02	/* "l": long or double.  */
#define	LONGDBL		0x04	/* "L/ll": long double or long long.  */
#define CHAR		0x08	/* "hh": 8 bit integer.  */
#define	SUPPRESS	0x10	/* Suppress assignment.  */
#define	POINTER		0x20	/* Weird %p pointer (`fake hex').  */
#define	NOSKIP		0x40	/* Do not skip blanks */

/* The following are used in numeric conversions only:
   SIGNOK, NDIGITS, DPTOK, and EXPOK are for floating point;
   SIGNOK, NDIGITS, PFXOK, and NZDIGITS are for integral.  */

#define	SIGNOK		0x80	/* "+/-" is (still) legal.  */
#define	NDIGITS		0x100	/* No digits detected.  */

#define	DPTOK		0x200	/* (Float) decimal point is still legal.  */
#define	EXPOK		0x400	/* (Float) exponent (e+3, etc) still legal.  */

#define	PFXOK		0x200	/* "0x" prefix is (still) legal.  */
#define	NZDIGITS	0x400	/* No zero digits detected.  */
#define	NNZDIGITS	0x800	/* No non-zero digits detected.  */

/* Conversion types.  */

#define	CT_CHAR		0	/* "%c" conversion.  */
#define	CT_CCL		1	/* "%[...]" conversion.  */
#define	CT_STRING	2	/* "%s" conversion.  */
#define	CT_INT		3	/* Integer, i.e., strtol.  */
#define	CT_UINT		4	/* Unsigned integer, i.e., strtoul.  */
#define	CT_FLOAT	5	/* Floating, i.e., strtod.  */

#define u_char unsigned char
#define u_long unsigned long
#define u_quad unsigned long long

extern u_char *__sccl (char *, u_char *fmt);

/* Macro to support positional arguments.  */
#define GET_ARG(n, ap, type) (va_arg ((ap), type))

#define MATCH_FAILURE	1
#define INPUT_FAILURE	2


/* All data needed to decode format string are kept in below struct.  */
struct _scan_data_t
{
  int flags;            /* Flags.  */
  int base;             /* Base.  */
  size_t width;         /* Width.  */
  int nassigned;        /* Number of assignments so far.  */
  int nread;            /* Number of chars read so far.  */
  char *ccltab;         /* Table used for [ format.  */
  int code;             /* Current conversion specifier.  */
  char buf[BUF];        /* Internal buffer for scan.  */
  /* Internal buffer for scan.  */
  int (*pfn_ungetc)(struct _reent*, int, FILE*);
  /* Internal buffer for scan.  */
  int (*pfn_refill)(struct _reent*, FILE*);
};

extern int
_scanf_chars (struct _reent *rptr,
	      struct _scan_data_t *pdata,
	      FILE *fp, va_list *ap);

extern int
_scanf_i (struct _reent *rptr,
	  struct _scan_data_t *pdata,
	  FILE *fp, va_list *ap);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

extern int
_scanf_float (struct _reent *rptr,
	      struct _scan_data_t *pdata,
	      FILE *fp, va_list *ap) _ATTRIBUTE((__weak__));


int
__wrap___ssvfscanf_r (struct _reent *rptr,
       register FILE *fp,
       char const *fmt0,
       va_list ap)
{
  register u_char *fmt = (u_char *) fmt0;
  register int c;		/* Character from format, or conversion.  */
  register char *p;		/* Points into all kinds of strings.  */
  char ccltab[256];		/* Character class table for %[...].  */
  va_list ap_copy;

  int ret;
  char *cp;

  struct _scan_data_t scan_data;

  _newlib_flockfile_start (fp);

  scan_data.nassigned = 0;
  scan_data.nread = 0;
  scan_data.ccltab = ccltab;
  scan_data.pfn_ungetc = _ungetc_r;
  scan_data.pfn_refill = __srefill_r;

  /* GCC PR 14577 at https://gcc.gnu.org/bugzilla/show_bug.cgi?id=14557 */
  va_copy (ap_copy, ap);

  for (;;)
    {
      if (*fmt == 0)
	goto all_done;

      if (isspace (*fmt))
	{
	  while ((fp->_r > 0 || !scan_data.pfn_refill(rptr, fp))
		 && isspace (*fp->_p))
	    {
	      scan_data.nread++;
	      fp->_r--;
	      fp->_p++;
	    }
	  fmt++;
	  continue;
	}
      if ((c = *fmt++) != '%')
	goto literal;

      scan_data.width = 0;
      scan_data.flags = 0;

      if (*fmt == '*')
	{
	  scan_data.flags |= SUPPRESS;
	  fmt++;
	}

      for (; is_digit (*fmt); fmt++)
	scan_data.width = 10 * scan_data.width + to_digit (*fmt);

      /* The length modifiers.  */
      p = "hlL";
      if ((cp = memchr (p, *fmt, 3)) != NULL) {
	    fmt++;
        if (*cp == 'l' && *fmt == 'l') {
            scan_data.flags |= LONGDBL;
            fmt++;
        } else {
            scan_data.flags |= (SHORT << (cp - p));
        }
      }

      /* Switch on the format.  continue if done; break once format
	 type is derived.  */
      c = *fmt++;
      switch (c)
	{
	case '%':
	literal:
	  if ((fp->_r <= 0 && scan_data.pfn_refill(rptr, fp)))
	    goto input_failure;
	  if (*fp->_p != c)
	    goto match_failure;
	  fp->_r--, fp->_p++;
	  scan_data.nread++;
	  continue;

	case 'p':
	  scan_data.flags |= POINTER;
	case 'x':
	case 'X':
	  scan_data.flags |= PFXOK;
	  scan_data.base = 16;
	  goto number;
	case 'd':
	case 'u':
	  scan_data.base = 10;
	  goto number;
	case 'i':
	  scan_data.base = 0;
	  goto number;
	case 'o':
	  scan_data.base = 8;
	number:
	  scan_data.code = (tolower(c) < 'o') ? CT_INT : CT_UINT;
	  break;

	case '[':
	  fmt = (u_char *) __sccl (ccltab, (unsigned char *) fmt);
	  scan_data.flags |= NOSKIP;
	  scan_data.code = CT_CCL;
	  break;
	case 'c':
	  scan_data.flags |= NOSKIP;
	  scan_data.code = CT_CHAR;
	  break;
	case 's':
	  scan_data.code = CT_STRING;
	  break;

	case 'n':
	  if (scan_data.flags & SUPPRESS)	/* ???  */
	    continue;

	  if (scan_data.flags & SHORT)
	    *GET_ARG (N, ap_copy, short *) = scan_data.nread;
	  else if (scan_data.flags & LONG)
	    *GET_ARG (N, ap_copy, long *) = scan_data.nread;
      else if (scan_data.flags & LONGDBL)
        *GET_ARG (N, ap_copy, long long*) = scan_data.nread;
	  else
	    *GET_ARG (N, ap_copy, int *) = scan_data.nread;

	  continue;

	/* Disgusting backwards compatibility hacks.	XXX.  */
	case '\0':		/* compat.  */
	  _newlib_flockfile_exit (fp);
	  va_end (ap_copy);
	  return EOF;

#ifdef FLOATING_POINT
	case 'e': case 'E':
	case 'f': case 'F':
	case 'g': case 'G':
	  scan_data.code = CT_FLOAT;
	  break;
#endif
	default:		/* compat.  */
	  scan_data.code = CT_INT;
	  scan_data.base = 10;
	  break;
	}

      /* We have a conversion that requires input.  */
      if ((fp->_r <= 0 && scan_data.pfn_refill (rptr, fp)))
	goto input_failure;

      /* Consume leading white space, except for formats that
	 suppress this.  */
      if ((scan_data.flags & NOSKIP) == 0)
	{
	  while (isspace (*fp->_p))
	    {
	      scan_data.nread++;
	      if (--fp->_r > 0)
		fp->_p++;
	      else if (scan_data.pfn_refill (rptr, fp))
		goto input_failure;
	    }
	  /* Note that there is at least one character in the
	     buffer, so conversions that do not set NOSKIP ca
	     no longer result in an input failure.  */
	}
      ret = 0;
      if (scan_data.code < CT_INT)
	ret = _scanf_chars (rptr, &scan_data, fp, &ap_copy);
      else if (scan_data.code < CT_FLOAT)
	ret = _scanf_i (rptr, &scan_data, fp, &ap_copy);
#ifdef FLOATING_POINT
      else if (_scanf_float)
	ret = _scanf_float (rptr, &scan_data, fp, &ap_copy);
#endif

      if (ret == MATCH_FAILURE)
	goto match_failure;
      else if (ret == INPUT_FAILURE)
	goto input_failure;
    }
input_failure:
  /* On read failure, return EOF failure regardless of matches; errno
     should have been set prior to here.  On EOF failure (including
     invalid format string), return EOF if no matches yet, else number
     of matches made prior to failure.  */
  _newlib_flockfile_exit (fp);
  va_end (ap_copy);
  return scan_data.nassigned && !(fp->_flags & __SERR) ? scan_data.nassigned
						       : EOF;
match_failure:
all_done:
  /* Return number of matches, which can be 0 on match failure.  */
  _newlib_flockfile_end (fp);
  va_end (ap_copy);
  return scan_data.nassigned;
}

int
__wrap__scanf_i (struct _reent *rptr,
	  struct _scan_data_t *pdata,
	  FILE *fp, va_list *ap)
{
#define CCFN_PARAMS	(struct _reent *, const char *, char **, int)
  /* Conversion function (strtol/strtoul).  */
  u_long (*ccfn)CCFN_PARAMS=0;
  u_quad (*ccfnq)CCFN_PARAMS=0;
  char *p;
  int n;
  char *xdigits = "A-Fa-f8901234567]";
  char *prefix_chars[3] = {"+-", "00", "xX"};

  /* Scan an integer as if by strtol/strtoul.  */
  unsigned width_left = 0;
  int skips = 0;

  ccfn = (pdata->code == CT_INT) ? (u_long (*)CCFN_PARAMS)_strtol_r : _strtoul_r;
  ccfnq = (pdata->code == CT_INT) ? (u_quad (*)CCFN_PARAMS)_strtoll_r : _strtoull_r;
#ifdef hardway
  if (pdata->width == 0 || pdata->width > BUF - 1)
#else
  /* size_t is unsigned, hence this optimisation.  */
  if (pdata->width - 1 > BUF - 2)
#endif
    {
      width_left = pdata->width - (BUF - 1);
      pdata->width = BUF - 1;
    }
  p = pdata->buf;
  pdata->flags |= NDIGITS | NZDIGITS | NNZDIGITS;

  /* Process [sign] [0] [xX] prefixes sequently.  */
  for (n = 0; n < 3; n++)
    {
      if (!memchr (prefix_chars[n], *fp->_p, 2))
	continue;

      if (n == 1)
	{
	  if (pdata->base == 0)
	    {
	      pdata->base = 8;
	      pdata->flags |= PFXOK;
	    }
	  pdata->flags &= ~(NZDIGITS | NDIGITS);
	}
      else if (n == 2)
	{
	  if ((pdata->flags & (PFXOK | NZDIGITS)) != PFXOK)
	    continue;
	  pdata->base = 16;

	  /* We must reset the NZDIGITS and NDIGITS
	     flags that would have been unset by seeing
	     the zero that preceded the X or x.

	     ??? It seems unnecessary to reset the NZDIGITS.  */
	  pdata->flags |= NDIGITS;
	}
      if (pdata->width-- > 0)
	{
	  *p++ = *fp->_p++;
	  fp->_r--;
	  if ((fp->_r <= 0 && pdata->pfn_refill (rptr, fp)))
	    goto match_end;
	}
    }

  if (pdata->base == 0)
    pdata->base = 10;

  /* The check is un-necessary if xdigits points to exactly the string:
     "A-Fa-f8901234567]".  The code is kept only for reading's sake.  */
#if 0
  if (pdata->base != 16)
#endif
  xdigits = xdigits + 16 - pdata->base;

  /* Initilize ccltab according to pdata->base.  */
  __sccl (pdata->ccltab, (unsigned char *) xdigits);
  for (; pdata->width; pdata->width--)
    {
      n = *fp->_p;
      if (pdata->ccltab[n] == 0)
	break;
      else if (n == '0' && (pdata->flags & NNZDIGITS))
	{
	  ++skips;
	  if (width_left)
	    {
	      width_left--;
	      pdata->width++;
	    }
	  goto skip;
	}
      pdata->flags &= ~(NDIGITS | NNZDIGITS);
      /* Char is legal: store it and look at the next.  */
      *p++ = *fp->_p;
skip:
      if (--fp->_r > 0)
	fp->_p++;
      else if (pdata->pfn_refill (rptr, fp))
	/* "EOF".  */
	break;
    }
  /* If we had only a sign, it is no good; push back the sign.
     If the number ends in `x', it was [sign] '0' 'x', so push back
     the x and treat it as [sign] '0'.
     Use of ungetc here and below assumes ASCII encoding; we are only
     pushing back 7-bit characters, so casting to unsigned char is
     not necessary.  */
match_end:
  if (pdata->flags & NDIGITS)
    {
      if (p > pdata->buf)
	pdata->pfn_ungetc (rptr, *--p, fp); /* "[-+xX]".  */

      if (p == pdata->buf)
	return MATCH_FAILURE;
    }
  if ((pdata->flags & SUPPRESS) == 0)
    {
      u_quad ul;
      *p = 0;
      if (!(pdata->flags & LONGDBL)) {
        ul = (*ccfn) (rptr, pdata->buf, (char **) NULL, pdata->base);
      } else {
        ul = (*ccfnq) (rptr, pdata->buf, (char **) NULL, pdata->base);
      }
      if (pdata->flags & POINTER)
	*GET_ARG (N, *ap, void **) = (void *) (uintptr_t) ul;
      else if (pdata->flags & SHORT)
	*GET_ARG (N, *ap, short *) = ul;
      else if (pdata->flags & LONG)
	*GET_ARG (N, *ap, long *) = ul;
      else if (pdata->flags & LONGDBL)
    *GET_ARG (N, *ap, long long*) = ul;
      else
	*GET_ARG (N, *ap, int *) = ul;
      
      pdata->nassigned++;
    }
  pdata->nread += p - pdata->buf + skips;
  return 0;
}

#pragma GCC diagnostic pop

#endif // PLATFORM_ID != PLATFORM_GCC
