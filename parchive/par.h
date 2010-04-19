/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef PAR_H
#define PAR_H

#include "types.h"

#define PAR_FIX_HEAD_SIZE	0x60
#define FILE_ENTRY_FIX_SIZE	0x38

struct par_s {
	u32 version;
	u32 client;
	md5 control_hash;
	md5 set_hash;
	i64 vol_number;
	i64 num_files;
	i64 file_list;
	i64 file_list_size;
	i64 data;
	i64 data_size;

	i64 control_hash_offset;
	pfile_t *files;
	pfile_t *volumes;
	u16 *filename;
	u16 *comment;
	file_t f;
};

struct pfile_entr_s {
	i64 size;
	i64 status;
	i64 file_size;
	md5 hash;
	md5 hash_16k;
	u16 filename[1];
};

struct pfile_s {
	pfile_t *next;
	i64 status;
	i64 file_size;
	md5 hash_16k;
	md5 hash;
	i64 vol_number;
	u16 *filename;
	file_t f;
	hfile_t *match;
	u16 *fnrs;
};

extern struct cmdline {
	int action;
	int loglevel;
	int volumes;	/*\ Number of volumes to create \*/

	int pervol : 1;	/*\ volumes is actually files per volume \*/
	int plus :1;	/*\ Turn on or off options (with + or -) \*/
	int move :1;	/*\ Move away files that are in the way \*/
	int fix :1;	/*\ Fix files with bad filenames \*/
	int usecase :1;	/*\ Compare filenames without case \*/
	int dupl :1;	/*\ Check for duplicate files \*/
	int add :1;	/*\ Don't add files to PXX volumes \*/
	int pxx :1;	/*\ Create PXX volumes \*/
	int ctrl :1;	/*\ Check/create control hash \*/
	int keep :1;	/*\ Keep broken files \*/
	int smart :1;	/*\ Try to be smart about filenames \*/
	int dash :1;	/*\ End of cmdline switches \*/
} cmd;

#define CMP_MD5(a,b) (!memcmp((a), (b), sizeof(md5)))

#define USE_FILE(p) ((p)->status & 0x1)

#endif /* PAR_H */
