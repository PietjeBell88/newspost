/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __UTILS_H__
#define __UTILS_H__

typedef struct{
	char *data;
	long length;
	long real_length;
}
Buff;

typedef struct Newspost_SList {
	void *data;
	struct Newspost_SList *next;
}
SList;

typedef struct {
	struct stat fileinfo;
	Buff *filename;
	n_uint32 crc;
	boolean *parts;
}
file_entry;

file_entry * file_entry_alloc();
file_entry * file_entry_free(file_entry *fe);

Buff *buff_getline(Buff *buff, FILE *file);
Buff *buff_add(Buff *buff, char *data, ... );
Buff * buff_free(Buff *buff);
Buff *buff_create(Buff *buff, char *data, ... );

SList *slist_next(SList *slist);
SList *slist_append(SList *slist, void *data);
SList *slist_prepend(SList *slist, void *data);
SList *slist_remove(SList *slist, void *data);
void slist_free(SList *slist);
int slist_length(SList *slist);

const char *n_basename(const char *path);

#endif /* __UTILS_H__ */
