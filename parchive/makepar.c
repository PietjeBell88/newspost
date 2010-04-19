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

/* Create PAR files */

#include "../base/newspost.h"
#include "../ui/ui.h"

#include "util.h"
#include "backend.h"
#include "makepar.h"
#include "rwpar.h"
#include "fileops.h"
#include "md5.h"

struct cmdline cmd;

/**
*** Private Declarations
**/

static int par_add_file(par_t *par, hfile_t *file);
static SList *par_make_pxx(par_t *par);

/**
*** Public Routines
**/

SList *
par_newspost_interface(newspost_data *data, SList *file_list) {
	par_t *par = NULL;
	file_entry *filedata = NULL;
	FILE *fp;
	char *fn;
	char md5[16];
	Buff *tmpstring = NULL;
	SList *parfiles;
	SList *pi;
	file_entry *fileinfo;

	ui_par_gen_start();

	/* Some defaults */
	memset(&cmd, 0, sizeof(cmd));
	cmd.pxx = 1;
	cmd.ctrl = 1;
	cmd.add = 1;
	cmd.usecase = 1;

	if (data->parnum > 0) {
		cmd.pervol = 0;
		cmd.volumes = data->parnum;
	}
	else {
		cmd.pervol = 1;
		cmd.volumes = data->filesperpar;
	}

	/* check if the name really ends in .par */
	if(strlen(data->par->data) > 4){
		fn = data->par->data;
		fn += (strlen(data->par->data) - 4);
		if (strncasecmp(fn, ".par", 4) != 0) {
			data->par = buff_add(data->par,".par");
		}
	}
	else{
		data->par = buff_add(data->par,".par");
	}

	tmpstring = buff_create(tmpstring, "%s/%s", data->tmpdir->data, data->par->data);
	data->par = buff_create(data->par, "%s", tmpstring->data);
	tmpstring = buff_free(tmpstring);

	par = read_par_header(unist(data->par->data), 1, 0, 0);
	if (par == NULL) {
		ui_par_gen_error();
		return NULL;
	}

	par->comment = malloc(STRING_BUFSIZE);
	sprintf((char *) par->comment, "Created by %s/%s %s",
		NEWSPOSTNAME, VERSION, NEWSPOSTURL);

	while (file_list != NULL) {
		filedata = (file_entry *) file_list->data;

		par_add_file(par,
			find_file_name(unist(filedata->filename->data), 1));
		ui_par_file_add_done(filedata->filename->data);
		
		file_list = slist_next(file_list);
	}

	ui_par_volume_create_start();
	if ((parfiles = par_make_pxx(par)) == NULL) {
		ui_par_gen_error();
		free_par(par);
		return NULL;
	}

	/* add the md5sum */
	pi = parfiles;
	while (pi != NULL) {
		filedata = (file_entry *) pi->data;
		fp = fopen(filedata->filename->data, "rb+");
		fseek(fp, 32, SEEK_SET);
		md5_stream(fp, md5);
		fseek(fp, 16, SEEK_SET);
		fwrite(md5, 16, 1, fp);
		fclose(fp);
		pi = slist_next(pi);
	}

	write_par_header(par);
	free(par->comment);
	free_par(par);

	fileinfo = file_entry_alloc();
	fileinfo->filename = buff_create(fileinfo->filename, 
					 "%s", data->par->data);
	stat(fileinfo->filename->data, &fileinfo->fileinfo);
	parfiles = slist_prepend(parfiles, fileinfo);
	chmod(fileinfo->filename->data, S_IRUSR | S_IWUSR);
	ui_par_volume_created(fileinfo->filename->data);

	return parfiles;
}

/**
*** Private Routines
**/

/*
 Add a data file to a PAR file
*/
static int
par_add_file(par_t *par, hfile_t *file)
{
	pfile_t *p, **pp;

	if (!file)
		return 0;
	if (!hash_file(file, HASH)) {
	  /*
		fprintf(stderr, "  %-40s - ERROR\n",
				p_basename(file->filename));
	  */
		return 0;
	}
	/* Check if the file exists */
	for (p = par->files; p; p = p->next) {
		switch (unicode_cmp(p->filename, file->filename)) {
		case 0:
			if (CMP_MD5(p->hash, file->hash)) {
/*
				fprintf(stderr, "  %-40s - EXISTS\n",
					p_basename(file->filename));
*/
			} else {
			  /*
				fprintf(stderr, "  %-40s - NAME CLASH\n",
					p_basename(file->filename));
			  */
			}
			return 0;
		case 1:
			if (CMP_MD5(p->hash, file->hash)) {
			  /*
				fprintf(stderr, "  %-40s - EXISTS\n",
					p_basename(file->filename));
			  */
				return 0;
			}
			break;
		}
	}

	/* Create new entry */
	CNEW(p, 1);
	p->filename = file->filename;
	p->match = file;
	p->file_size = file->file_size;
	COPY(p->hash, file->hash, sizeof(md5));
	COPY(p->hash_16k, file->hash_16k, sizeof(md5));
	if (cmd.add)
		p->status |= 0x01;

	/* Insert in alphabetically correct place */
	for (pp = &par->files; *pp; pp = &((*pp)->next))
		if (unicode_gt((*pp)->filename, p->filename))
			break;
	p->next = *pp;
	*pp = p;

	/* fprintf(stderr, "  %-40s - OK\n", p_basename(file->filename)); */

	return 1;
}

/*
 Create the PAR volumes from the description in the PAR archive
*/
static SList *
par_make_pxx(par_t *par)
{
	pfile_t *p, *v;
	int M, i;
        SList *parlist;

	if (par->vol_number) {
		CNEW(v, 1);
		v->match = find_file_name(par->filename, 0);
		if (!v->match)
			v->match = find_volume(par->filename, par->vol_number);
		v->vol_number = par->vol_number;
		if (v->match)
			v->filename = v->match->filename;
		par->volumes = v;
	} else {
		if (cmd.volumes <= 0)
			return 0;

		M = cmd.volumes;
		if (cmd.pervol) {
			for (M = 0, p = par->files; p; p = p->next)
				if (USE_FILE(p))
					M++;
			M = ((M - 1) / cmd.volumes) + 1;
		}

		/* Create volume file entries */
		for (i = 1; i <= M; i++) {
			CNEW(v, 1);
			v->match = find_volume(par->filename, i);
			v->vol_number = i;
			if (v->match)
				v->filename = v->match->filename;
			v->next = par->volumes;
			par->volumes = v;
		}
	}

	/* fprintf(stderr, "\n\nCreating PAR volumes:\n"); */
	for (p = par->files; p; p = p->next)
		if (USE_FILE(p))
			find_file(p, 1);

	for (v = par->volumes; v; v = v->next)
		v->fnrs = file_numbers(&par->files, &par->files);

	parlist = restore_files(par->files, par->volumes);

	return parlist;
}
