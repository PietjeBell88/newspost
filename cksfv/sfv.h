/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __SFV_H__
#define __SFV_H__

#include "../base/newspost.h"

n_uint32 crc32(const char *buf, size_t len, n_uint32 crc);
void calculate_crcs(SList *file_list);
void newsfv(SList *file_list, newspost_data *np_data);

#endif /* __SFV_H__ */
