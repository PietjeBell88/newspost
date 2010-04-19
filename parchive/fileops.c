/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001  Willem Monsuwe (willem@stack.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *
 * Modified for Newspost by Jim Faulkner and William McBrine
 */

/*
  File operations, in a separate file because these are probably
   going to cause the most portability problems.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "md5.h"
#include "fileops.h"
#include "util.h"
#include "par.h"

/**
*** Private Declarations
**/

static int do_open(file_t f);

static int do_close(file_t f);

static int do_seek(file_t f);

static file_t file_open_ascii(const char *path, int wr);

static void unistr(const char *str, u16 *buf);

/**
*** Public Routines
**/

/*
 Translate an ASCII string into unicode
 Returns an allocated string
*/
u16 *
make_uni_str(const char *str)
{
	u16 *uni;

	NEW(uni, strlen(str) +1);
	unistr(str, uni);
	return uni;
}

/*
 Translate a unicode string into ASCII
 Returns static string, which is overwritten at next call
*/
char *
stuni(const u16 *str)
{
	i64 i;
	static char *buf = 0;
	static i64 bufsize = 0;

	/* Count the length */
	for (i = 0; str[i]; i++)
		;
	if ((i + 1) > bufsize) {
		bufsize = i + 1;
		RENEW(buf, bufsize);
	}
	/* For now, just copy the low byte */
	for (i = 0; str[i]; i++)
		buf[i] = str[i];
	buf[i] = 0;
	return buf;
}

/*
 Translate an ASCII string into unicode
 Returns static string, which is overwritten at next call
*/
u16 *
unist(const char *str)
{
	i64 i;
	static u16 *buf = 0;
	static i64 bufsize = 0;

	/* Count the length */
	for (i = 0; str[i]; i++)
		;
	if ((i + 1) > bufsize) {
		bufsize = i + 1;
		RENEW(buf, bufsize);
	}
	/* For now, just copy the low byte */
	for (i = 0; str[i]; i++)
		buf[i] = str[i];
	buf[i] = 0;
	return buf;
}

i64
uni_copy(u16 *dst, u16 *src, i64 n)
{
	i64 i;
	for (i = 0; src[i] && (i < (n - 1)); i++)
		dst[i] = src[i];
	dst[i] = 0;
	return i;
}

u16 *
unicode_copy(u16 *str)
{
	u16 *ret, *p;

	if (!str) {
		NEW(ret, 1);
		ret[0] = 0;
		return ret;
	}
	for (p = str; *p; p++)
		;
	NEW(ret, (p - str) + 1);
	COPY(ret, str, (p - str) + 1);
	return ret;
}

file_t
file_open(const u16 *path, int wr)
{
	return file_open_ascii(stuni(path), wr);
}

int
file_close(file_t f)
{
	int i;
	if (!f) return 0;
	i = do_close(f);
	free(f->name);
	free(f);
	return i;
}

int
file_delete(u16 *file)
{
	return remove(stuni(file));
}

int
file_seek(file_t f, i64 off)
{
	f->s_off = off;
	return do_seek(f);
}


char *
complete_path(char *path)
{
	return path;
}

/* Read a directory.
   Returns a linked list of file entries.
*/
hfile_t *
read_dir(char *dir)
{
	DIR *d;
	struct dirent *de;
	hfile_t *rd = 0, **rdptr = &rd;
	u16 *p;
	int l, i;
	char *dr;

	dir = complete_path(dir);

	l = 0;
	for (i = 0; dir[i]; i++)
		if (dir[i] == DIR_SEP)
			l = i + 1;

	NEW(dr, l + 1);
	memcpy(dr, dir, l);
	dr[l] = 0;

	d = opendir(l ? dr : ".");
	if (d) {
		while ((de = readdir(d))) {
			CNEW(*rdptr, 1);
			NEW(p, l + strlen(de->d_name) + 1);
			unistr(dr, p);
			unistr(de->d_name, p + l);
			(*rdptr)->filename = p;
			rdptr = &((*rdptr)->next);
		}
		closedir(d);
	}

	free(dr);
	return rd;
}

i64
file_read(file_t f, void *buf, i64 n)
{
	i64 i;
	if (!f) return 0;
	if (do_open(f) < 0)
		return 0;
	i = fread(buf, 1, n, f->f);
	if (i > 0) {
		f->off += i;
		f->s_off = f->off;
	}
	return i;
}

i64
file_write(file_t f, void *buf, i64 n)
{
	i64 i;
	if (!f) return 0;
	if (do_open(f) < 0)
		return 0;
	i = fwrite(buf, 1, n, f->f);
	if (i > 0) {
		f->off += i;
		f->s_off = f->off;
	}
	return i;
}

/* Calculate md5 sums on a file */
i64
file_md5(u16 *file, md5 block)
{
	FILE *f;
	i64 i;

	f = fopen(stuni(file), "rb");
	if (!f) return 0;
	i = md5_stream(f, block);
	fclose(f);
	return i;
}

int
file_md5_buffer(u16 *file, md5 block, u8 *buf, i64 size)
{
	file_t f;
	i64 s;

	f = file_open(file, 0);
	if (!f) return 0;
	s = file_read(f, buf, size);
	file_close(f);
	if (s < 0) return 0;
	return (md5_buffer((char *) buf, s, block) != 0);
}

/* Calculate the md5sum of a file from offset 'off',
   put it at offset 'md5off'
   Also, check the file size.  Return 0 on failure.
*/
int
file_add_md5(file_t f, i64 md5off, i64 off, i64 len)
{
	md5 hash;
	i64 i;

	if (!f) return 0;
	f->s_off = off;
	if (do_open(f) < 0)
		return 0;
	i = md5_stream(f->f, hash);
	if (i < 0)
		return 0;
	f->off += i;
	f->s_off = f->off;
	/* Filepointer should be at EOF now */
	if (f->off != len)
		return 0;
	if (file_seek(f, md5off) < 0)
		return 0;
	if (file_write(f, hash, sizeof(hash)) < 0)
		return 0;
	return 1;
}

/**
*** Private Routines
**/

/*
 Translate an ASCII string into unicode, write it onto the buffer
*/
static void
unistr(const char *str, u16 *buf)
{
	do *buf++ = *str; while (*str++);
}

static file_t
file_open_ascii(const char *path, int wr)
{
	file_t f;

	CNEW(f, 1);
	NEW(f->name, strlen(path) + 1);
	strcpy(f->name, path);
	f->wr = wr;
	return f;
}

static int
do_seek(file_t f)
{
	if (f->off == f->s_off) return 0;
	if (!f->f) return 0;
	if (fseek(f->f, f->s_off, SEEK_SET) < 0)
		return -1;
	f->off = f->s_off;
	return 0;
}

static int
do_close(file_t f)
{
	int i;
	if (!f->f) return 0;
	i = fclose(f->f);
	f->f = 0;
	return i;
}

static int
do_open(file_t f)
{
	static file_t openfiles = 0;
	int i;
	while (!f->f) {
		/* This is so complicated to make sure we don't overwrite */
		i = open(f->name, f->wr ? O_RDWR|O_CREAT|O_EXCL : O_RDONLY,
				0666);
		if (i >= 0) f->f = fdopen(i, f->wr ? "w+b" : "rb");
		if (!f->f) {
			if ((errno != EMFILE) && (errno != ENFILE))
				return -1;
			if (!openfiles)
				return -1;
			while (!openfiles->f)
				openfiles = openfiles->next;
			do_close(openfiles);
			openfiles = openfiles->next;
		} else {
			f->off = 0;
			if (!f->wr) {
				f->next = openfiles;
				openfiles = f;
			}
		}
	}
	return do_seek(f);
}
