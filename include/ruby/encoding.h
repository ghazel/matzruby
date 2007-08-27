/**********************************************************************

  encoding.h -

  $Author: matz $
  $Date: 2007-05-24 11:49:41 +0900 (Thu, 24 May 2007) $
  created at: Thu May 24 11:49:41 JST 2007

  Copyright (C) 2007 Yukihiro Matsumoto

**********************************************************************/

#ifndef RUBY_ENCODING_H
#define RUBY_ENCODING_H 1

#include "ruby/oniguruma.h"

#define ENCODING_INLINE_MAX 15
#define ENCODING_MASK (FL_USER8|FL_USER9|FL_USER10|FL_USER11)
#define ENCODING_SHIFT (FL_USHIFT+8)
#define ENCODING_SET(obj,i) do {\
    RBASIC(obj)->flags &= ~ENCODING_MASK;\
    RBASIC(obj)->flags |= i << ENCODING_SHIFT;\
} while (0)
#define ENCODING_GET(obj) ((RBASIC(obj)->flags & ENCODING_MASK)>>ENCODING_SHIFT)

typedef OnigEncodingType rb_encoding;

int rb_enc_to_index(rb_encoding*);
rb_encoding* rb_enc_get(VALUE);
rb_encoding* rb_enc_check(VALUE,VALUE);
void rb_enc_associate(VALUE, rb_encoding*);
void rb_enc_copy(VALUE, VALUE);

VALUE rb_enc_str_new(const char*, long len, rb_encoding*);
long rb_enc_strlen(const char*, const char*, rb_encoding*);
char* rb_enc_nth(const char*, const char*, int, rb_encoding*);

/* index -> rb_encoding */
rb_encoding* rb_enc_from_index(int idx);

/* name -> rb_encoding */
rb_encoding * rb_enc_find(const char *name);

/* encoding -> name */
#define rb_enc_name(enc) (enc)->name

/* encoding -> minlen/maxlen */
#define rb_enc_mbminlen(enc) (enc)->min_enc_len
#define rb_enc_mbmaxlen(enc) (enc)->max_enc_len

/* ptr,encoding -> mbclen */
int rb_enc_mbclen(const char*, rb_encoding*);

/* code,encoding -> codelen */
int rb_enc_codelen(int, rb_encoding*);

/* code,ptr,encoding -> write buf */
#define rb_enc_mbcput(c,buf,enc) ONIGENC_CODE_TO_MBC(enc,c,(UChar*)buf)

/* ptr,ptr,encoding -> codepoint */
#define rb_enc_codepoint(p,e,enc) ONIGENC_MBC_TO_CODE(enc,(UChar*)p,(UChar*)e) 

/* ptr, ptr, encoding -> prev_char */
#define rb_enc_prev_char(s,p,enc) onigenc_get_prev_char_head(enc,(UChar*)s,(UChar*)p)

#define rb_enc_isascii(c,enc) ONIGENC_IS_CODE_ASCII(c)
#define rb_enc_isalpha(c,enc) ONIGENC_IS_CODE_ALPHA(enc,c)
#define rb_enc_islower(c,enc) ONIGENC_IS_CODE_LOWER(enc,c)
#define rb_enc_isupper(c,enc) ONIGENC_IS_CODE_UPPER(enc,c)
#define rb_enc_isalnum(c,enc) ONIGENC_IS_CODE_ALNUM(enc,c)
#define rb_enc_isprint(c,enc) ONIGENC_IS_CODE_PRINT(enc,c)
#define rb_enc_isspace(c,enc) ONIGENC_IS_CODE_SPACE(enc,c)
#define rb_enc_isdigit(c,enc) ONIGENC_IS_CODE_DIGIT(enc,c)

int rb_enc_toupper(int c, rb_encoding *enc);
int rb_enc_tolower(int c, rb_encoding *enc);
ID rb_intern3(const char*, long, rb_encoding*);

#endif /* RUBY_ENCODING_H */
