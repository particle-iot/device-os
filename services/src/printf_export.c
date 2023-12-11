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
#include "printf_export.h"

// Copy of nano-vfprint.c, nano-vfprintf_i.c nano-vfprintf_local.h
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


#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit (c) <= 9)

extern int _printf_common (struct _reent *data,
		struct _prt_data_t *pdata,
		int *realsz,
		FILE *fp,
		int (*pfunc)(struct _reent *, FILE *,
			     const char *, size_t len));

#define quad_t long long
#define u_quad_t unsigned long long
typedef quad_t * quad_ptr_t;
typedef void *void_ptr_t;
typedef char *   char_ptr_t;
typedef long *   long_ptr_t;
typedef int  *   int_ptr_t;
typedef short *  short_ptr_t;

/* Flags used during conversion.  */
#define	ALT		0x001		/* Alternate form.  */
#define	LADJUST		0x002		/* Left adjustment.  */
#define	ZEROPAD		0x004		/* Zero (as opposed to blank) pad.  */
#define PLUSSGN		0x008		/* Plus sign flag.  */
#define SPACESGN	0x010		/* Space flag.  */
#define	HEXPREFIX	0x020		/* Add 0x or 0X prefix.  */
#define	SHORTINT	0x040		/* Short integer.  */
#define	LONGINT		0x080		/* Long integer.  */
#define	LONGDBL		0x100		/* Long double.  */
/* ifdef _NO_LONGLONG, make QUADINT equivalent to LONGINT, so
   that %lld behaves the same as %ld, not as %d, as expected if:
   sizeof (long long) = sizeof long > sizeof int.  */
#define QUADINT	  0x200
#define FPT		0x400		/* Floating point number.  */
/* Define as 0, to make SARG and UARG occupy fewer instructions.  */
# define CHARINT	0

/* Macros to support positional arguments.  */
#define GET_ARG(n, ap, type) (va_arg ((ap), type))

/* To extend shorts properly, we need both signed and unsigned
   argument extraction methods.  Also they should be used in nano-vfprintf_i.c
   and nano-vfprintf_float.c only, since ap is a pointer to va_list.  */
#define	SARG(flags) \
	(flags&QUADINT ? GET_ARG (N, (*ap), long long) : \
       flags&LONGINT ? GET_ARG (N, (*ap), long) : \
	    flags&SHORTINT ? (long)(short)GET_ARG (N, (*ap), int) : \
	    flags&CHARINT ? (long)(signed char)GET_ARG (N, (*ap), int) : \
	    (long)GET_ARG (N, (*ap), int))
#define	UARG(flags) \
	(flags&QUADINT ? GET_ARG (N, (*ap), unsigned long long) : \
       flags&LONGINT ? GET_ARG (N, (*ap), u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)GET_ARG (N, (*ap), int) : \
	    flags&CHARINT ? (u_long)(unsigned char)GET_ARG (N, (*ap), int) : \
	    (u_long)GET_ARG (N, (*ap), u_int))

/* BEWARE, these `goto error' on error. And they are used
   in more than one functions.

   Following macros are each referred about twice in printf for integer,
   so it is not worth to rewrite them into functions. This situation may
   change in the future.  */
#define PRINT(ptr, len) {		\
	if (pfunc (data, fp, (ptr), (len)) == EOF) \
		goto error;		\
}

#define PAD(howmany, ch) {             \
       int temp_i = 0;                 \
       while (temp_i < (howmany))      \
       {                               \
               if (pfunc (data, fp, &(ch), 1) == EOF) \
                       goto error;     \
               temp_i++;               \
       }			       \
}

#define STRING_ONLY

#ifdef STRING_ONLY
# define __SPRINT __ssputs_r
extern int __ssputs_r (struct _reent *, FILE *, const char *, size_t);
#else
# define __SPRINT __sfputs_r
#endif

/* Do not need FLUSH for all sprintf functions.  */
#ifdef STRING_ONLY
# define FLUSH()
#else
# define FLUSH()
#endif

int __wrap__svfprintf_r (struct _reent *data,
       FILE * fp,
       const char *fmt0,
       va_list ap)
{
  register char *fmt;	/* Format string.  */
  register int n = 0, m;	/* Handy integers (short term usage).  */
  register char *cp;	/* Handy char pointer (short term usage).  */
  const char *flag_chars;
  struct _prt_data_t prt_data;	/* All data for decoding format string.  */
  va_list ap_copy;

  /* Output function pointer.  */
  int (*pfunc)(struct _reent *, FILE *, const char *, size_t len);

  pfunc = __SPRINT;

#ifndef STRING_ONLY
  /* Initialize std streams if not dealing with sprintf family.  */
  CHECK_INIT (data, fp);
  _newlib_flockfile_start (fp);

  /* Sorry, fprintf(read_only_file, "") returns EOF, not 0.  */
  if (cantwrite (data, fp))
    {
      _newlib_flockfile_exit (fp);
      return (EOF);
    }

#else
  /* Create initial buffer if we are called by asprintf family.  */
  if (fp->_flags & __SMBF && !fp->_bf._base)
    {
      fp->_bf._base = fp->_p = _malloc_r (data, 64);
      if (!fp->_p)
	{
	  data->_errno = ENOMEM;
	  return EOF;
	}
      fp->_bf._size = 64;
    }
#endif

  fmt = (char *)fmt0;
  prt_data.ret = 0;
  prt_data.blank = ' ';
  prt_data.zero = '0';

  /* GCC PR 14577 at https://gcc.gnu.org/bugzilla/show_bug.cgi?id=14557 */
  va_copy (ap_copy, ap);

  /* Scan the format for conversions (`%' character).  */
  for (;;)
    {
      cp = fmt;
      while (*fmt != '\0' && *fmt != '%')
	fmt += 1;

      if ((m = fmt - cp) != 0)
	{
	  PRINT (cp, m);
	  prt_data.ret += m;
	}
      if (*fmt == '\0')
	goto done;

      fmt++;		/* Skip over '%'.  */

      prt_data.flags = 0;
      prt_data.width = 0;
      prt_data.prec = -1;
      prt_data.dprec = 0;
      prt_data.l_buf[0] = '\0';
#ifdef FLOATING_POINT
      prt_data.lead = 0;
#endif
      /* The flags.  */
      /*
       * ``Note that 0 is taken as a flag, not as the
       * beginning of a field width.''
       *	-- ANSI X3J11
       */
      flag_chars = "#-0+ ";
      for (; (cp = memchr (flag_chars, *fmt, 5)); fmt++)
	prt_data.flags |= (1 << (cp - flag_chars));

      if (prt_data.flags & SPACESGN)
	prt_data.l_buf[0] = ' ';

      /*
       * ``If the space and + flags both appear, the space
       * flag will be ignored.''
       *	-- ANSI X3J11
       */
      if (prt_data.flags & PLUSSGN)
	prt_data.l_buf[0] = '+';

      /* The width.  */
      if (*fmt == '*')
	{
	  /*
	   * ``A negative field width argument is taken as a
	   * - flag followed by a positive field width.''
	   *	-- ANSI X3J11
	   * They don't exclude field widths read from args.
	   */
	  prt_data.width = GET_ARG (n, ap_copy, int);
	  if (prt_data.width < 0)
	    {
	      prt_data.width = -prt_data.width;
	      prt_data.flags |= LADJUST;
	    }
	  fmt++;
	}
      else
        {
	  for (; is_digit (*fmt); fmt++)
	    prt_data.width = 10 * prt_data.width + to_digit (*fmt);
	}

      /* The precision.  */
      if (*fmt == '.')
	{
	  fmt++;
	  if (*fmt == '*')
	    {
	      fmt++;
	      prt_data.prec = GET_ARG (n, ap_copy, int);
	      if (prt_data.prec < 0)
		prt_data.prec = -1;
	    }
	  else
	    {
	      prt_data.prec = 0;
	      for (; is_digit (*fmt); fmt++)
		prt_data.prec = 10 * prt_data.prec + to_digit (*fmt);
	    }
	}

      /* The length modifiers.  */
      flag_chars = "hlL";
      if ((cp = memchr (flag_chars, *fmt, 3)) != NULL)
	{
      fmt++;
      // %llu/%lld/%llx etc support
      if (*fmt == 'l' && *cp == 'l') {
        prt_data.flags |= QUADINT;
        fmt++;
      } else {
        prt_data.flags |= (SHORTINT << (cp - flag_chars));
      }
	}

      /* The conversion specifiers.  */
      prt_data.code = *fmt++;
      cp = memchr ("efgEFG", prt_data.code, 6);
#ifdef FLOATING_POINT
      /* If cp is not NULL, we are facing FLOATING POINT NUMBER.  */
      if (cp)
	{
	  /* Consume floating point argument if _printf_float is not
	     linked.  */
	  if (_printf_float == NULL)
	    {
	      if (prt_data.flags & LONGDBL)
		GET_ARG (N, ap_copy, _LONG_DOUBLE);
	      else
		GET_ARG (N, ap_copy, double);
	    }
	  else
            n = _printf_float (data, &prt_data, fp, pfunc, &ap_copy);
	}
      else
#endif
	n = _printf_i (data, &prt_data, fp, pfunc, &ap_copy);

      if (n == -1)
	goto error;

      prt_data.ret += n;
    }
done:
  FLUSH ();
error:
#ifndef STRING_ONLY
  _newlib_flockfile_end (fp);
#endif
  va_end (ap_copy);
  return (__sferror (fp) ? EOF : prt_data.ret);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
int __wrap__printf_i (struct _reent *data, struct _prt_data_t *pdata, FILE *fp,
	   int (*pfunc)(struct _reent *, FILE *, const char *, size_t len),
	   va_list *ap) {
  /* Field size expanded by dprec.  */
  int realsz;
  u_quad_t _uquad;
  int base;
  int n;
  char *cp = pdata->buf + BUF;
  char *xdigs = "0123456789ABCDEF";

  /* Decoding the conversion specifier.  */
  switch (pdata->code)
    {
    case 'c':
      *--cp = GET_ARG (N, *ap, int);
      pdata->size = 1;
      goto non_number_nosign;
    case 'd':
    case 'i':
      _uquad = SARG (pdata->flags);
      if ((quad_t)_uquad < 0)
	{
	  _uquad = -_uquad;
	  pdata->l_buf[0] = '-';
	}
      base = 10;
      goto number;
    case 'u':
    case 'o':
      _uquad = UARG (pdata->flags);
      base = (pdata->code == 'o') ? 8 : 10;
      goto nosign;
    case 'X':
      pdata->l_buf[2] = 'X';
      goto hex;
    case 'p':
      /*
       * ``The argument shall be a pointer to void.  The
       * value of the pointer is converted to a sequence
       * of printable characters, in an implementation-
       * defined manner.''
       *	-- ANSI X3J11
       */
      pdata->flags |= HEXPREFIX;
      if (sizeof (void*) > sizeof (int))
	pdata->flags |= LONGINT;
      /* NOSTRICT.  */
    case 'x':
      pdata->l_buf[2] = 'x';
      xdigs = "0123456789abcdef";
hex:
      _uquad = UARG (pdata->flags);
      base = 16;
      if (pdata->flags & ALT)
	pdata->flags |= HEXPREFIX;

      /* Leading 0x/X only if non-zero.  */
      if (_uquad == 0)
	pdata->flags &= ~HEXPREFIX;

      /* Unsigned conversions.  */
nosign:
      pdata->l_buf[0] = '\0';
      /*
       * ``... diouXx conversions ... if a precision is
       * specified, the 0 flag will be ignored.''
       *	-- ANSI X3J11
       */
number:
      if ((pdata->dprec = pdata->prec) >= 0)
	pdata->flags &= ~ZEROPAD;

      /*
       * ``The result of converting a zero value with an
       * explicit precision of zero is no characters.''
       *	-- ANSI X3J11
       */
      if (_uquad != 0 || pdata->prec != 0)
	{
	  do
	    {
	      *--cp = xdigs[_uquad % base];
	      _uquad /= base;
	    }
	  while (_uquad);
	}
      /* For 'o' conversion, '#' increases the precision to force the first
	 digit of the result to be zero.  */
      if (base == 8 && (pdata->flags & ALT) && pdata->prec <= pdata->size)
	*--cp = '0';

      pdata->size = pdata->buf + BUF - cp;
      break;
    case 'n':
      if (pdata->flags & LONGINT)
	*GET_ARG (N, *ap, long_ptr_t) = pdata->ret;
      else if (pdata->flags & SHORTINT)
	*GET_ARG (N, *ap, short_ptr_t) = pdata->ret;
      else
	*GET_ARG (N, *ap, int_ptr_t) = pdata->ret;
    case '\0':
      pdata->size = 0;
      break;
    case 's':
      cp = GET_ARG (N, *ap, char_ptr_t);
      /* Precision gives the maximum number of chars to be written from a
	 string, and take prec == -1 into consideration.
	 Use normal Newlib approach here to support case where cp is not
	 nul-terminated.  */
      char *p = memchr (cp, 0, pdata->prec);

      if (p != NULL)
	pdata->prec = p - cp;

      pdata->size = pdata->prec;
      goto non_number_nosign;
    default:
      /* "%?" prints ?, unless ? is NUL.  */
      /* Pretend it was %c with argument ch.  */
      *--cp = pdata->code;
      pdata->size = 1;
non_number_nosign:
      pdata->l_buf[0] = '\0';
      break;
    }

    /* Output.  */
    n = _printf_common (data, pdata, &realsz, fp, pfunc);
    if (n == -1)
      goto error;

    PRINT (cp, pdata->size);
    /* Left-adjusting padding (always blank).  */
    if (pdata->flags & LADJUST)
      PAD (pdata->width - realsz, pdata->blank);

    return (pdata->width > realsz ? pdata->width : realsz);
error:
    return -1;
}

#pragma GCC diagnostic pop

#endif // PLATFORM_ID != PLATFORM_GCC
