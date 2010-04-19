/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __YENCODE_H__
#define __YENCODE_H__

#define YENC_LINE_LENGTH 128

long yencode(FILE *infile, char *outbuf, int maxlines, n_uint32 *crc);

#endif /* __YENCODE_H__ */
