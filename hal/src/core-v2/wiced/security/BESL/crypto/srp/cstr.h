/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef _CSTR_H_
#define _CSTR_H_

/* A general-purpose string "class" for C */

#if     !defined(P)
#ifdef  __STDC__
#define P(x)    x
#else
#define P(x)    ()
#endif
#endif

/*    For building dynamic link libraries under windows, windows NT
 *    using MSVC1.5 or MSVC2.0
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stddef.h"

/* Arguments to allocator methods ordered this way for compatibility */
typedef struct cstr_alloc_st {
  void * (* alloc)(size_t n);
  void (* free)(void * p);
  void * heap;
} cstr_allocator;

typedef struct cstr_st {
  char * data;    /* Okay to access data and length fields directly */
  int length;
  int cap;
  int ref;    /* Simple reference counter */
  cstr_allocator * allocator;
} cstr;

void cstr_set_allocator P((cstr_allocator * alloc));

cstr * cstr_new P((void));
cstr * cstr_new_alloc P((cstr_allocator * alloc));
cstr * cstr_dup P((const cstr * str));
cstr * cstr_dup_alloc P((const cstr * str, cstr_allocator * alloc));
cstr * cstr_create P((const char * s));
cstr * cstr_createn P((const char * s, int len));

void cstr_free P((cstr * str));
void cstr_clear_free P((cstr * str));
void cstr_use P((cstr * str));
void cstr_empty P((cstr * str));
int cstr_copy P((cstr * dst, const cstr * src));
int cstr_set P((cstr * str, const char * s));
int cstr_setn P((cstr * str, const char * s, int len));
int cstr_set_length P((cstr * str, int len));
int cstr_append P((cstr * str, const char * s));
int cstr_appendn P((cstr * str, const char * s, int len));
int cstr_append_str P((cstr * dst, const cstr * src));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CSTR_H_ */
