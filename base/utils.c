/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Author: Jim Faulkner <newspost@sdf.lonestar.org>
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
 */

#include <stdarg.h>

#include "newspost.h"
#include "../ui/errors.h"

static long buff_data_length(char *fmt, va_list ap);

file_entry * file_entry_alloc(){
	file_entry * fe = malloc(sizeof(file_entry));
	fe->parts = NULL;
	fe->filename = NULL;
	fe->rwlock = NULL;
	fe->parts_posted = 0;
	fe->post_started = FALSE;
	return fe;
}

file_entry * file_entry_free(file_entry *fe){
	if(fe != NULL){
		if(fe->parts != NULL)
			free(fe->parts);
		if(fe->filename != NULL)
			buff_free(fe->filename);
		if(fe->rwlock != NULL) {
			pthread_rwlock_destroy(fe->rwlock);
			free(fe->rwlock);
		}
		free(fe);
	}
	return NULL;
}

Buff * buff_getline(Buff *buff, FILE *file){
	char c = fgetc(file);
	buff = buff_free(buff);
	while(TRUE){
		if((c == '\n')
		   || (c == EOF)) break;
		buff = buff_add(buff, "%c", c);
		c = fgetc(file);
	}
	
	return buff;
}

/* Accepts subset of printf() syntax. 
   Supports %c, %s, %i, %%, and %0*i
   More can be added in buff_data_length() */
Buff *buff_create(Buff *buff, char *data, ... ){
	va_list ap;
	long datalength;
	
	/* find out the total length of the data */
	va_start(ap, data);
	datalength = buff_data_length(data, ap);
	va_end(ap);

	/* this function frees whatever *buff points to! */
	if(buff != NULL)
		buff = buff_free(buff);

	if(datalength == 0) return NULL;

	/* initialize the Buff */
	buff = malloc(sizeof(Buff));
	buff->length = datalength;
	
	/* determine the real length to make this buffer */
	/* real length is more than length... this reduces the number of realloc()s */
	for(buff->real_length = 2 ; 
	    buff->real_length <= datalength; 
	    buff->real_length = buff->real_length * 2){}
	buff->data = malloc(buff->real_length);
	
	/* write the data to the memory we just malloced */
	va_start(ap, data);
	vsprintf(buff->data,data,ap);
	va_end(ap);
	buff->data[buff->length] = '\0';

	return buff;
}

/* Accepts subset of printf() syntax. 
   Supports %c, %s, %i, %%, and %0*i
   More can be added in buff_data_length() */
Buff *buff_add(Buff *buff, char *data, ... ){
	va_list ap;
	long datalength;

	/* find out the total length of the data */
	va_start(ap, data);
	datalength = buff_data_length(data, ap);
	va_end(ap);

	va_start(ap, data);
	if(buff == NULL){
		if(datalength == 0) return NULL;
		
		/* initialize the Buff */
		buff = malloc(sizeof(Buff));
		buff->length = datalength;
		
		/* determine the real length to make this buffer */
		for(buff->real_length = 2 ; 
		    buff->real_length <= datalength; 
		    buff->real_length = buff->real_length * 2){}
		buff->data = malloc(buff->real_length);
		
		/* write the data to the memory we just malloced */
		vsprintf(buff->data,data,ap);
	}
	else{
		/* save the old length (where to start writing data)
		   and update the Buff */
		int oldlength = buff->length;
		buff->length += datalength;
		
		/* realloc more memory if necessary */
		if(buff->length >= buff->real_length){
			for(;
			    buff->real_length <= buff->length;
			    buff->real_length = buff->real_length * 2){}
			buff->data = realloc(buff->data, buff->real_length);
		}
		
		/* append the data to the buff */
		vsprintf((buff->data + oldlength), data, ap);
	}
	va_end(ap);
	buff->data[buff->length] = '\0';
	
	return buff;
}

Buff * buff_free(Buff *buff){
	if(buff != NULL){
		free(buff->data);
		free(buff);
	}
	return NULL;
}

/* Find last occurrence of '/' or '\\'; return rest of string */
const char *n_basename(const char *path) {
	const char *current;

	for (current = path; *current; current++)
		if (('/' == *current) || ('\\' == *current))
			path = current + 1;

	return path;
}

SList *slist_next(SList *slist) {
	return slist->next;
}

SList *slist_remove(SList *slist, void *data) {
	SList *si = slist;
	SList *prev = NULL;

	while (si != NULL) {
		if (si->data == data) {
			if (prev == NULL) {
				si = si->next;
				free(slist);
				return si;
			}
			prev->next = si->next;
			free(si);
			return slist;
		}
		prev = si;
		si = si->next;
	}
	return slist;
}

SList *slist_append(SList *slist, void *data) {
	SList *si;
	if (slist == NULL) {
		slist = (SList *) malloc(sizeof(SList));
		slist->data = data;
		slist->next = NULL;
		return slist;
	}
	else {
		si = slist;
		while (si->next != NULL)
			si = si->next;

		si->next = (SList *) malloc(sizeof(SList));
		si = si->next;
		si->data = data;
		si->next = NULL;
		return slist;
	}
}

SList *slist_prepend(SList *slist, void *data) {
	SList *si;
	if (slist == NULL) {
		slist = (SList *) malloc(sizeof(SList));
		slist->data = data;
		slist->next = NULL;
		return slist;		
	}
	else {
		si = (SList *) malloc(sizeof(SList));
		si->data = data;
		si->next = slist;
		return si;
	}
}

void slist_free(SList *slist) {
	SList *si;
	while (slist != NULL) {
		si = slist;
		slist = slist->next;
		free(si);
	}
}

int slist_length(SList *slist) {
	int i = 0;
	while (slist != NULL) {
		i++;
		slist = slist->next;
	}
	return i;
}

/**
*** Private Routines
**/

static long buff_data_length(char *fmt, va_list ap){
	char *pi;
	char lengthstr[64]; /* all numbers will fit in here */
	long retval = 0;
	
	/* go through the string, looking for '%' */
	for(pi = fmt; *pi; pi++){
		if(*pi == '%'){
			switch(*++pi){
			case 's':
				/* add the length of the string argument */
				retval += strlen(va_arg(ap, char *));
				break;

			case '%':
				/* add one.  its just a '%'! */
				retval++;
				break;

			case 'c':
				/* add one. we have to call va_arg()
				   to go to the next argument */
				va_arg(ap, int);
				retval++;
				break;

			case 'i':
				/* add the length of the number */
				sprintf(lengthstr,"%i",va_arg(ap, int));
				retval += strlen(lengthstr);
				break;

			case '0':
				/* add the number of zeroes used in zero-padding */
				if(*++pi == '*'){
					if(*++pi == 'i'){
						retval += va_arg(ap, int);
						va_arg(ap, int);
						break;
					}
				}

			/* This only works with a subset of printf syntax.
			   If you want to add more you must handle it above. */
			default:
				fprintf(stderr, "\nInternal Error: see utils.c\n");
				exit(EXIT_UNKNOWN);
			}
		}
		else{
			/* a plain character */
			retval++;
		}
	}
	return retval;
}
