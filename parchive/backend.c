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

/* Backend operations */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "util.h"
#include "rwpar.h"
#include "fileops.h"
#include "rs.h"
#include "backend.h"
#include "md5.h"

/*
  Static directory list
*/
hfile_t *hfile = 0;


/**
*** Public Routines
**/

char *
p_basename(u16 *path)
{
	u16 *ret;

	for (ret = path; *path; path++)
		if (*path == DIR_SEP)
			ret = path + 1;
	return stuni(ret);
}

/*
  Order two unicode strings (caseless)
  Return 1 if a > b
*/
int
unicode_gt(u16 *a, u16 *b)
{
	for (; *a || *b; a++, b++) {
		if (tolower(*a) > tolower(*b)) return 1;
		if (tolower(*a) < tolower(*b)) return 0;
	}
	return 0;
}

/*
 Compare two unicode strings
 -1: Strings are not equal
  0: Strings are equal
  1: Strings only differ in upper/lowercase (doesn't happen with cmd.usecase)
*/
int
unicode_cmp(u16 *a, u16 *b)
{
	for (; *a == *b; a++, b++)
		if (!*a) return 0;
	if (!cmd.usecase)
		for (; tolower(*a) == tolower(*b); a++, b++)
			if (!*a) return 1;
	return -1;
}

hfile_t *
hfile_add(u16 *filename)
{
	hfile_t **pp;

	for (pp = &hfile; *pp; pp = &((*pp)->next))
		;
	CNEW(*pp, 1);
	(*pp)->filename = unicode_copy(filename);
	return (*pp);
}

/*
 Read in a directory and add it to the static directory structure
*/
void
hash_directory(char *dir)
{
	hfile_t *p, *q, **pp;

	/* only add new items */
	for (p = read_dir(dir); p; ) {
		for (pp = &hfile; *pp; pp = &((*pp)->next))
			if (!unicode_cmp(p->filename, (*pp)->filename))
				break;
		if (*pp) {
			q = p;
			p = p->next;
			free(q);
		} else {
			*pp = p;
			p = p->next;
			(*pp)->next = 0;
		}
	}
}

/*
 Calculate md5 sums for a file, but only once
*/
int
hash_file(hfile_t *file, char type)
{
	i64 s;
	u8 buf[16384];

	if (type < HASH16K) return 1;
	if (file->hashed < HASH16K) {
		if (!file_md5_buffer(file->filename, file->hash_16k,
					buf, sizeof(buf)))
			return 0;
		file->hashed = HASH16K;
	}
	if (type < HASH) return 1;
	if (file->hashed < HASH) {
		s = file_md5(file->filename, file->hash);
		if (s >= 0) {
			file->hashed = HASH;
			if (!file->file_size)
				file->file_size = s;
		}
	}
	return 1;
}


/*
 Find a file in the static directory structure that matches the md5 sums.
 Try matching filenames first
*/
int
find_file(pfile_t *file, int displ)
{
	hfile_t *p;
	int cm; 
	/* int corr = 0; */

	if (file->match) return 1;

	/* Check filename (caseless) and then check md5 hash */
	for (p = hfile; p; p = p->next) {
		cm = unicode_cmp(p->filename, file->filename);
		if (cm < 0) continue;
		if (!hash_file(p, HASH)) {
/*
			if (displ) {
				fprintf(stderr, "      ERROR: %s",
						p_basename(p->filename));
				perror(" ");
			}
			corr = 1;
*/
			continue;
		}
		if (CMP_MD5(p->hash, file->hash)) {
			if (!cm || !file->match)
				file->match = p;
			continue;
		}
		/*
		if (displ)
			fprintf(stderr, "      ERROR: %s: Failed md5 sum\n",
					p_basename(p->filename));
		corr = 1;
		*/
	}
	if (file->match) {
	  /*
		if (displ)
			fprintf(stderr, "  %-40s - OK\n",
				p_basename(file->filename));
	  */
		if (!displ || !cmd.dupl)
			return 1;
	}

	/* Try to match md5 hash on all files */
	for (p = hfile; p; p = p->next) {
		if (file->match == p)
			continue;
		if (!hash_file(p, HASH16K))
			continue;
		if (!CMP_MD5(p->hash_16k, file->hash_16k))
			continue;
		if (!hash_file(p, HASH))
			continue;
		if (!CMP_MD5(p->hash, file->hash))
			continue;
		if (!file->match) {
			file->match = p;
/*
			if (displ) {
				fprintf(stderr, "  %-40s - FOUND",
					p_basename(file->filename));
				fprintf(stderr, ": %s\n",
					p_basename(p->filename));
			}
*/
			if (!displ || !cmd.dupl)
				return 1;
		}
/*
		fprintf(stderr, "    Duplicate: %s",
			stuni(file->match->filename));
		fprintf(stderr, " == %s\n",
			p_basename(p->filename));
*/
	}
/*
	if (!file->match && displ)
		fprintf(stderr, "  %-40s - %s\n",
			p_basename(file->filename),
			corr ? "CORRUPT" : "NOT FOUND");
*/
	return (file->match != 0);
}

/*
 Find a file in the static directory structure
*/
hfile_t *
find_file_name(u16 *path, int displ)
{
	hfile_t *p, *ret = 0;

	hash_directory(stuni(path));
	path = unist(complete_path(stuni(path)));

	/* Check filename (caseless) and then check md5 hash */
	for (p = hfile; p; p = p->next) {
		switch (unicode_cmp(p->filename, path)) {
		case 1:
			if (ret) break;
		case 0:
			ret = p;
		}
	}
/*
	if (!ret && displ)
		fprintf(stderr, "  %-40s - NOT FOUND\n", p_basename(path));
*/
	return ret;
}

/*
 Find a volume in the static directory structure.
  Base the name on the given filename and volume number.
  Create it if it's not found.
*/
hfile_t *
find_volume(u16 *name, i64 vol)
{
	u16 *filename;
	i64 i;
	hfile_t *p, *ret = 0;
	int nd, v;

	if (vol < 1)
		return 0;
	nd = 2;
	for (v = 100; vol >= v; v *= 10)
		nd++;
	for (i = 0; name[i]; i++)
		;
	if ((name[i-1] < '0') || (name[i-1] > '9')) {
		i = i - 2;
	} else {
		while ((name[i-1] >= '0') && (name[i-1] <= '9'))
			i--;
	}
	i += nd;
	NEW(filename, i + 1);
	uni_copy(filename, name, i);
	filename[i] = 0;
	v = vol;
	while (--nd >= 0) {
		filename[--i] = '0' + v % 10;
		v /= 10;
	}

	for (p = hfile; p; p = p->next) {
		switch (unicode_cmp(p->filename, filename)) {
		case 1:
			if (ret) break;
		case 0:
			ret = p;
		}
	}
	if (!ret)
		ret = hfile_add(filename);
	free(filename);
	return ret;
}

u16 *
file_numbers(pfile_t **list, pfile_t **files)
{
	int i, j;
	pfile_t *p, **qq;
	u16 *fnrs;

	for (i = 0, p = *files; p; p = p->next)
		if (USE_FILE(p))
			i++;
	NEW(fnrs, i + 1);
	for (i = 0; *files; ) {
		/* Look for a match */
		for (j = 1, qq = list; *qq; qq = &((*qq)->next), j++) {
			if ((*files)->file_size != (*qq)->file_size) continue;
			if (!CMP_MD5((*files)->hash, (*qq)->hash)) continue;
			break;
		}
		if (USE_FILE(*files))
			fnrs[i++] = j;
		/* No match ? Move the file entry to the tail of the list */
		if (!*qq) {
			*qq = *files;
			*files = (*files)->next;
			(*qq)->next = 0;
		} else {
			files = &((*files)->next);
		}
	}
	fnrs[i] = 0;
	return fnrs;
}

