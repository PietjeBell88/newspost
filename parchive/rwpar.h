/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef READWRITEPAR_H
#define READWRITEPAR_H

#include "../base/newspost.h"
#include "types.h"
#include "par.h"

par_t * read_par_header(u16 *file, int create, i64 vol, int silent);
void free_par(par_t *par);
file_t write_par_header(par_t *par);
SList * restore_files(pfile_t *files, pfile_t *volumes);

/* void dump_par(par_t *par); */

#endif /* READWRITEPAR_H */
