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

/* Read and write PAR files */

#include "../base/newspost.h"
#include "../ui/ui.h"

#include "util.h"
#include "rwpar.h"
#include "fileops.h"
#include "rs.h"
#include "md5.h"
#include "backend.h"

/* Endianless fixing code */

static i64
read_i64(void *data)
{
	int i;
	i64 r = 0;
	u8 *ptr = data;

	for (i = sizeof(i64); --i >= 0; ) {
		r <<= 8;
		r += (i64)ptr[i];
	}
	return r;
}

static u32
read_u32(void *data)
{
	int i;
	u32 r = 0;
	u8 *ptr = data;

	for (i = sizeof(u32); --i >= 0; ) {
		r <<= 8;
		r += (u32)ptr[i];
	}
	return r;
}

/* Read N bytes of little-endian u16s */
static void
read_u16s(u16 *str, void *data, i64 n)
{
	u8 *ptr = data;
	while (--n >= 0) {
		*str = ptr[0] + (ptr[1] << 8);
		str++;
		ptr += 2;
	}
}

static void
write_i64(i64 v, void *data)
{
	size_t i;
	u8 *ptr = data;

	for (i = 0; i < sizeof(i64); i++) {
		ptr[i] = v & 0xFF;
		v >>= 8;
	}
}

static void
write_u32(u32 v, void *data)
{
	size_t i;
	u8 *ptr = data;

	for (i = 0; i < sizeof(u32); i++) {
		ptr[i] = v & 0xFF;
		v >>= 8;
	}
}

/* Write N bytes of little-endian u16s */
static void
write_u16s(u16 *str, void *data, i64 n)
{
	u8 *ptr = data;
	while (--n >= 0) {
		ptr[0] = (*str) & 0xff;
		ptr[1] = ((*str) >> 8) & 0xff;
		str++;
		ptr += 2;
	}
}

/* Change endianness to host byte order
   NB: This is a fix in place.  Don't call this twice!
*/
static void
par_endian_read(par_t *par)
{
	par->version = read_u32(&par->version);
	par->client = read_u32(&par->client);
	par->vol_number = read_i64(&par->vol_number);
	par->num_files = read_i64(&par->num_files);
	par->file_list = read_i64(&par->file_list);
	par->file_list_size = read_i64(&par->file_list_size);
	par->data = read_i64(&par->data);
	par->data_size = read_i64(&par->data_size);
}

static void
par_endian_write(par_t *par, void *data)
{
	par_t *p = (par_t *)data;
	memcpy(p, par, PAR_FIX_HEAD_SIZE);
	write_u32(par->version, &p->version);
	write_u32(par->client, &p->client);
	write_i64(par->vol_number, &p->vol_number);
	write_i64(par->num_files, &p->num_files);
	write_i64(par->file_list, &p->file_list);
	write_i64(par->file_list_size, &p->file_list_size);
	write_i64(par->data, &p->data);
	write_i64(par->data_size, &p->data_size);
}


static u16 uni_empty[] = { 0 };

static i64
uni_sizeof(u16 *str)
{
	i64 l;
	for (l = 0; str[l]; l++)
		;
	return (2 * l);
}

/*
 Return a pointer just past the last occurrence of '/' in a unicode string
  (somewhat like strrchr)
*/
static u16 *
uni_strip(u16 *str)
{
	u16 *ret;

	for (ret = str; *str; str++)
		if (*str == DIR_SEP)
			ret = str + 1;
	return ret;
}

/*
 Read in a PAR file entry to a file struct
*/
static i64
read_pfile(pfile_t *file, u8 *ptr, u16 *path, i64 pl)
{
	i64 i, l;
	pfile_entr_t *pf;

	pf = ((pfile_entr_t *)ptr);

	i = read_i64(&pf->size);
	file->status = read_i64(&pf->status);
	file->file_size = read_i64(&pf->file_size);
	COPY(file->hash, pf->hash, sizeof(md5));
	COPY(file->hash_16k, pf->hash_16k, sizeof(md5));
	l = (i - FILE_ENTRY_FIX_SIZE) / 2;
	NEW(file->filename, pl + l + 1);
	COPY(file->filename, path, pl);
	read_u16s(file->filename + pl, &pf->filename, l);
	file->filename[l + pl] = 0;

	return i;
}

/*
 Make a list of pointers into a list of file entries
*/
static pfile_t *
read_pfiles(file_t f, i64 size, u16 *path)
{
	pfile_t *files = 0, **fptr = &files;
	u8 *buf;
	i64 i, pl;

	for (pl = i = 0; path[i]; i++)
		if (path[i] == DIR_SEP)
			pl = i + 1;

	NEW(buf, size);
	size = file_read(f, buf, size);

	/* The list size is at the start of the block */
	i = 0;
	/* Loop over the entries; the size of an entry is at the start */
	while (i < size) {
		CNEW(*fptr, 1);
		i += read_pfile(*fptr, buf + i, path, pl);
		fptr = &((*fptr)->next);
	}
	free(buf);
	return files;
}

/*
 Create a new PAR file struct
*/
static par_t *
create_par_header(u16 *file, i64 vol)
{
	par_t *par;

	CNEW(par, 1);
	par->version = 0x00010000;
	par->client = PAR_CLIENT;
	par->vol_number = vol;
	par->filename = unicode_copy(file);
	par->comment = uni_empty;
	par->control_hash_offset = 0x20;

	return par;
}

/*
 Read in a PAR file, and return it into a newly allocated struct
 (to be freed with free_par())
*/
par_t *
read_par_header(u16 *file, int create, i64 vol, int silent)
{
	par_t par, *r;
	char *path;

	memset(&par, 0, sizeof(par));

	hash_directory(stuni(file));
	path = complete_path(stuni(file));

	par.f = file_open(file, 0);
	/* Read in the first part of the struct, it fits directly on top */
	if (file_read(par.f, &par, PAR_FIX_HEAD_SIZE) < PAR_FIX_HEAD_SIZE) {
		if (!create || (errno != ENOENT)) {
/*
			if (!silent)
				perror("Error reading PAR file");
*/
			file_close(par.f);
			return 0;
		}
		if (!vol) {
			/* Guess volume number from last digits */
			u16 *p;
			for (p = file; *p; p++)
				;
			while ((--p >= file) && (*p >= '0') && (*p <= '9'))
				;
			while (*++p)
				vol = vol * 10 + (*p - '0');
		}
		return create_par_header(file, vol);
	}
	par_endian_read(&par);

	par.control_hash_offset = 0x20;
	par.filename = file;
	/*
	if (!silent && !par_control_check(&par)) {
		file_close(par.f);
		return 0;
	}
	*/

	file_seek(par.f, par.file_list);

	par.filename = make_uni_str(path);

	/* Read in the filelist. */
	par.files = read_pfiles(par.f, par.file_list_size, par.filename);

	file_seek(par.f, par.data);

	if (par.vol_number == 0) {
		CNEW(par.comment, (par.data_size / 2) + 1);
		file_read(par.f, par.comment, par.data_size);
	}

	file_close(par.f);
	par.f = 0;

	par.volumes = 0;

	NEW(r, 1);
	COPY(r, &par, 1);
/*
	if (cmd.loglevel > 1)
		dump_par(r);
*/
	return r;
}

static void
free_file_list(pfile_t *list)
{
	pfile_t *next;

	while (list) {
		if (list->f) file_close(list->f);
		if (list->fnrs) free(list->fnrs);
		next = list->next;
		free(list);
		list = next;
	}
}

void
free_par(par_t *par)
{
	free_file_list(par->files);
	free_file_list(par->volumes);
	free(par->filename);
	if (par->f) file_close(par->f);
	free(par);
}

/*
 Write out a PAR file entry from a file struct
*/
static i64
write_pfile(pfile_t *file, pfile_entr_t *pf)
{
	u16 *name;
	i64 i;

	name = uni_strip(file->filename);
	i = uni_sizeof(name);

	write_i64(FILE_ENTRY_FIX_SIZE + i, &pf->size);
	write_i64(file->status, &pf->status);
	write_i64(file->file_size, &pf->file_size);
	COPY(pf->hash, file->hash, sizeof(md5));
	COPY(pf->hash_16k, file->hash_16k, sizeof(md5));
	write_u16s(name, &pf->filename, i / 2);
	return FILE_ENTRY_FIX_SIZE + i;
}

/*
 Write a list of file entries to a file
*/
static i64
write_file_entries(file_t f, pfile_t *files)
{
	i64 tot, t, m;
	pfile_t *p;
	pfile_entr_t *pfe;

	tot = m = 0;
	for (p = files; p; p = p->next) {
		t = FILE_ENTRY_FIX_SIZE + uni_sizeof(uni_strip(p->filename));
		tot += t;
		if (m < t) m = t;
	}
	pfe = (pfile_entr_t *)malloc(m);
	if (f) {
		for (p = files; p; p = p->next) {
			t = write_pfile(p, pfe);
			file_write(f, pfe, t);
		}
	}
	free(pfe);
	return tot;
}

/*
 Write out a PAR volume header
*/
file_t
write_par_header(par_t *par)
{
	file_t f;
	par_t data;
	pfile_t *p;
	int i;
	md5 *hashes;

	/* Open output file */
	f = file_open(par->filename, 1);
	
	if (!f) {
	/*
		fprintf(stderr, "      WRITE ERROR: %s: ",
				p_basename(par->filename));
		perror("");
	*/
		return 0;
	}
	
	par->file_list = PAR_FIX_HEAD_SIZE;
	par->file_list_size = write_file_entries(0, par->files);
	par->data = par->file_list + par->file_list_size;

	if (par->vol_number == 0) {
		par->data_size = uni_sizeof(par->comment);
	} else {
		for (i = 0, p = par->files; p; p = p->next, i++) {
			if (par->data_size < p->file_size)
				par->data_size = p->file_size;
		}
	}
	/* Calculate set hash */
	par->num_files = 0;
	for (i = 0, p = par->files; p; p = p->next) {
		par->num_files++;
		if (USE_FILE(p))
			i++;
	}
	NEW(hashes, i);
	for (i = 0, p = par->files; p; p = p->next) {
		if (!USE_FILE(p))
			continue;
		COPY(hashes[i], p->hash, sizeof(md5));
		i++;
	}
	md5_buffer((char *)hashes, i * sizeof(md5), par->set_hash);
	free(hashes);
/*
	if (cmd.loglevel > 1)
		dump_par(par);
*/
	par_endian_write(par, &data);

	file_write(f, "PAR\0\0\0\0\0", 8);
	file_write(f, &data, (PAR_FIX_HEAD_SIZE - 8));
	write_file_entries(f, par->files);

	if (par->vol_number == 0) {
		file_write(f, par->comment, par->data_size);
		if (cmd.ctrl) {
			if (!file_add_md5(f, 0x0010, 0x0020,
					par->data + par->data_size))
			{
/*
				fprintf(stderr, "      ERROR: %s:",
						p_basename(par->filename));
				perror("");
				fprintf(stderr, "  %-40s - FAILED\n",
						p_basename(par->filename));
*/
				file_close(f);
				f = 0;
				if (!cmd.keep) file_delete(par->filename);
			}
		}
		if (f) file_close(f);
	}
	return f;
}

/*
 Restore missing files with recovery volumes
*/
SList *
restore_files(pfile_t *files, pfile_t *volumes)
{
	int N, M, i;
	xfile_t *in, *out;
	pfile_t *p, *v, **pp, **qq;
	int fail = 0;
	i64 size;
	pfile_t *mis_f, *mis_v;
	file_entry * fileinfo;
        SList * parlist = NULL;

	/* Separate out missing files */
	p = files;
	size = 0;
	pp = &files;
	qq = &mis_f;
	*pp = *qq = 0;
	for (i = 1; p; p = p->next, i++) {
		p->vol_number = i;
		if (!USE_FILE(p))
			continue;
		if (p->file_size > size)
			size = p->file_size;
		if (!find_file(p, 0)) {
			NEW(*qq, 1);
			COPY(*qq, p, 1);
			qq = &((*qq)->next);
			*qq = 0;
		} else {
			NEW(*pp, 1);
			COPY(*pp, p, 1);
			(*pp)->next = 0;
			pp = &((*pp)->next);
			*pp = 0;
		}
	}

	/* Separate out missing volumes */
	p = volumes;
	pp = &volumes;
	qq = &mis_v;
	*pp = *qq = 0;
	for (; p; p = p->next) {
		if (p->vol_number && !(p->f)) {
			NEW(*qq, 1);
			COPY(*qq, p, 1);
			qq = &((*qq)->next);
			*qq = 0;
		} else {
			NEW(*pp, 1);
			COPY(*pp, p, 1);
			pp = &((*pp)->next);
			*pp = 0;
		}
	}

	/* Count existing files and volumes */
	for (N = 0, p = files; p; p = p->next)
		N++;
	for (v = volumes; v; v = v->next, N++)
		N++;

	/* Count missing files and volumes */
	for (M = 0, p = mis_f; p; p = p->next)
		M++;
	for (v = mis_v; v; v = v->next, N++)
		M++;

	NEW(in, N + 1);
	NEW(out, M + 1);

	/* Fill in input files */
	for (i = 0, p = files; p; p = p->next) {
		p->f = file_open(p->match->filename, 0);
		if (!p->f) {
/*
			fprintf(stderr, "      ERROR: %s:",
					p_basename(p->match->filename));
			perror("");
*/
			continue;
		}
		in[i].filenr = p->vol_number;
		in[i].files = 0;
		in[i].size = p->file_size;
		in[i].f = p->f;
		i++;
	}
	/* Fill in input volumes */
	for (v = volumes; v; v = v->next) {
		in[i].filenr = v->vol_number;
		in[i].files = v->fnrs;
		in[i].size = v->file_size;
		in[i].f = v->f;
		i++;
	}
	in[i].filenr = 0;

	/* Fill in output files */
	for (i = 0, p = mis_f; p; p = p->next) {
		/* Open output file */
		p->f = file_open(p->filename, 1);
		if (!p->f) {
		  /*
			fprintf(stderr, "      ERROR: %s: ",
				p_basename(p->filename));
			perror("");
			fprintf(stderr, "  %-40s - NOT RESTORED\n",
				p_basename(p->filename));
		  */
			continue;
		}
		out[i].size = p->file_size;
		out[i].filenr = p->vol_number;
		out[i].files = 0;
		out[i].f = p->f;
		i++;
	}

	/* Fill in output volumes */
	for (v = mis_v; v; v = v->next) {
		par_t *par;

		par = create_par_header(v->filename, v->vol_number);
		if (!par) {
/*
			fprintf(stderr, "  %-40s - FAILED\n",
					p_basename(v->match->filename));
*/
			continue;
		}
		/* Copy file list into par file */
		par->files = files;
		par->data_size = size;
		v->f = write_par_header(par);
		par->files = 0;
		if (!v->f) {
/*
			fprintf(stderr, "  %-40s - FAILED\n",
					p_basename(par->filename));
*/
			fail |= 1;
			free_par(par);
			continue;
		}
		v->match = hfile_add(par->filename);
		v->filename = v->match->filename;
		v->file_size = par->data + par->data_size;
		out[i].size = par->data_size;
		out[i].filenr = v->vol_number;
		out[i].files = v->fnrs;
		out[i].f = v->f;
		free_par(par);
		i++;
	}
	out[i].filenr = 0;

	if (!recreate(in, out))
		fail |= 1;

	free(in);
	free(out);

	/* Check resulting data files */
	for (p = mis_f; p; p = p->next) {
		if (!p->f) continue;
		file_close(p->f);
		p->f = 0;
		p->match = hfile_add(p->filename);
		if (!hash_file(p->match, HASH)) {
/*
			fprintf(stderr, "      ERROR: %s:",
					p_basename(p->filename));
			perror("");
			fprintf(stderr, "  %-40s - NOT RESTORED\n",
					p_basename(p->filename));
*/
			fail |= 1;
			if (!cmd.keep) file_delete(p->filename);
			continue;
		}
		if ((p->match->file_size == 0) && (p->file_size != 0)) {
		  /*
			fprintf(stderr, "  %-40s - NOT RESTORED\n",
					p_basename(p->filename));
		  */
			fail |= 1;
			if (!cmd.keep) file_delete(p->filename);
			continue;
		}
		if (!CMP_MD5(p->match->hash, p->hash)) {
		  /*
			fprintf(stderr, "      ERROR: %s: Failed md5 check\n",
					p_basename(p->filename));
			fprintf(stderr, "  %-40s - NOT RESTORED\n",
					p_basename(p->filename));
		  */
			fail |= 1;
			if (!cmd.keep) file_delete(p->filename);
			continue;
		}
		/*
		fprintf(stderr, "  %-40s - RECOVERED\n",
				p_basename(p->filename));
		*/
	}

	/* Check resulting volumes */
	for (v = mis_v; v; v = v->next) {
		if (!v->f) continue;
		if (!file_add_md5(v->f, 0x0010, 0x0020, v->file_size)) {
/*
			fprintf(stderr, "  %-40s - FAILED\n",
					p_basename(v->filename));
*/
			fail |= 1;
			file_close(v->f);
			v->f = 0;
			if (!cmd.keep) file_delete(v->filename);
			continue;
		}

                fileinfo = file_entry_alloc();
                fileinfo->filename = 
			buff_create(fileinfo->filename,"%s",stuni(v->filename));
		stat(stuni(v->filename),&fileinfo->fileinfo);
                parlist = slist_prepend(parlist,fileinfo);
		file_close(v->f);
		v->f = 0;
		chmod(stuni(v->filename),S_IRUSR|S_IWUSR);
                ui_par_volume_created(stuni(v->filename));
		/* fprintf(stderr, "  %-40s - OK\n", p_basename(v->filename)); */
	}

	while ((p = files)) {
		files = p->next;
		free(p);
	}
	while ((p = volumes)) {
		volumes = p->next;
		free(p);
	}
	while ((p = mis_f)) {
		mis_f = p->next;
		free(p);
	}
	while ((p = mis_v)) {
		mis_v = p->next;
		free(p);
	}

	if (fail) {
	  /* fprintf(stderr, "\nErrors occurred.\n\n"); */
		return NULL;
	}
	return parlist;
}
