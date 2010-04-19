/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef BACKEND_H
#define BACKEND_H

#include "types.h"
#include "par.h"

char *p_basename(u16 *path);
int unicode_cmp(u16 *a, u16 *b);
int unicode_gt(u16 *a, u16 *b);
hfile_t *hfile_add(u16 *filename);
void hash_directory(char *dir);
int hash_file(hfile_t *file, char type);
int find_file(pfile_t *file, int displ);
hfile_t *find_file_name(u16 *path, int displ);
hfile_t *find_volume(u16 *name, i64 vol);
u16 *file_numbers(pfile_t **list, pfile_t **files);
par_t *find_all_par_files(void);
int par_control_check(par_t *par);

#endif /* BACKEND_H */
