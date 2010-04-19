/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef TYPES_H
#define TYPES_H

#include "../base/newspost.h"

typedef n_uint8  u8;
typedef n_uint16 u16;
typedef n_uint32 u32;
typedef n_int64  i64;
typedef u8 md5[16];

typedef struct par_s par_t;
typedef struct pxx_s pxx_t;
typedef struct pfile_entr_s pfile_entr_t;
typedef struct pfile_s pfile_t;
typedef struct hfile_s hfile_t;
typedef struct sub_s sub_t;

typedef struct file_s *file_t;

#endif /* TYPES_H */
