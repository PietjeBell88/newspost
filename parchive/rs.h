/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef REEDSOLOMON_H
#define REEDSOLOMON_H

typedef struct xfile_s xfile_t;

struct xfile_s {
	i64 size;
	file_t f;
	u16 filenr;
	u16 *files;
};

int recreate(xfile_t *in, xfile_t *out);

#endif /* REEDSOLOMON_H */
