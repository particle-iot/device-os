/****************************************************************************
 * include/string.h
 *
 *   Copyright (C) 2007-2012, 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_STRING_H
#define __INCLUDE_STRING_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stddef.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Compatibility definitions */

#ifndef CONFIG_ARCH_BZERO
# define bzero(s,n) (void)memset(s,0,n)
#endif

#define bcopy(b1,b2,len) (void)memmove(b2,b1,len)

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

FAR char  *strdup(FAR const char *s);
FAR char  *strndup(FAR const char *s, size_t size);
FAR const char *strerror(int);
size_t     strlen(FAR const char *);
size_t     strnlen(FAR const char *, size_t);
FAR char  *strcat(FAR char *, FAR const char *);
FAR char  *strncat(FAR char *, FAR const char *, size_t);
int        strcmp(FAR const char *, FAR const char *);
int        strncmp(FAR const char *, FAR const char *, size_t);
int        strcasecmp(FAR const char *, FAR const char *);
int        strncasecmp(FAR const char *, FAR const char *, size_t);
FAR char  *strcpy(char *dest, FAR const char *src);
FAR char  *stpcpy(char *dest, FAR const char *src);
FAR char  *strncpy(char *, FAR const char *, size_t);
FAR char  *strpbrk(FAR const char *, FAR const char *);
FAR char  *strchr(FAR const char *s, int c);
FAR char  *strrchr(FAR const char *s, int c);
size_t     strspn(FAR const char *, FAR const char *);
size_t     strcspn(FAR const char *, FAR const char *);
FAR char  *strstr(FAR const char *, FAR const char *);
FAR char  *strcasestr(FAR const char *, FAR const char *);
FAR char  *strtok(FAR char *, FAR const char *);
FAR char  *strtok_r(FAR char *, FAR const char *, FAR char **);
size_t     strlcpy(char *, const char *, size_t);

FAR void  *memchr(FAR const void *s, int c, size_t n);
FAR void  *memccpy(FAR void *s1, FAR const void *s2, int c, size_t n);
int        memcmp(FAR const void *s1, FAR const void *s2, size_t n);
FAR void  *memcpy(FAR void *dest, FAR const void *src, size_t n);
FAR void  *memmove(FAR void *dest, FAR const void *src, size_t count);
FAR void  *memset(FAR void *s, int c, size_t n);

#undef EXTERN
#if defined(__cplusplus)
}
#endif
#endif /* __INCLUDE_STRING_H */
