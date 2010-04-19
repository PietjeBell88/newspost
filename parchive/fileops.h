/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef FILEOPS_H
#define FILEOPS_H

#define DIR_SEP '/'

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "types.h"

struct hfile_s {
	hfile_t *next;
	md5 hash_16k;
	md5 hash;
	i64 file_size;
	u16 *filename;
	char *dir;
	u8 hashed;
};

struct file_s {
	file_t next;
	FILE *f;
	char *name;
	i64 off, s_off;
	int wr;
};

#define HASH16K 1
#define HASH 2

u16 *make_uni_str(const char *str);
char *stuni(const u16 *str);
u16 *unist(const char *str);
i64 uni_copy(u16 *dst, u16 *src, i64 n);
u16 * unicode_copy(u16 *str);
file_t file_open(const u16 *path, int wr);
int file_close(file_t f);
int file_delete(u16 *file);
int file_seek(file_t f, i64 off);
i64 file_md5(u16 *file, md5 block);
int file_md5_buffer(u16 *file, md5 block, u8 *buf, i64 size);
int file_add_md5(file_t f, i64 md5off, i64 off, i64 len);
char * complete_path(char *path);
hfile_t *read_dir(char *dir);
i64 file_read(file_t f, void *buf, i64 n);
i64 file_write(file_t f, void *buf, i64 n);

#endif /* FILEOPS_H */
