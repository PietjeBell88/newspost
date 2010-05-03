/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Authors: Jim Faulkner <newspost@sdf.lonestar.org>
 *          and William McBrine <wmcbrine@users.sf.net>
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

#include "newspost.h"
#include "../ui/ui.h"
#include "socket.h"
#include "nntp.h"
#include "encode.h"
#include "../cksfv/sfv.h"
#include "../parchive/parintrf.h"

/**
*** Private Declarations
**/

static int post_text_file(newspost_data *data, SList *file_list);

static int encode_and_post(newspost_data *data, SList *file_list,
			    SList *parfiles);

static SList *preprocess(newspost_data *data, SList *file_list);

static Buff *read_text_file(Buff * text_buffer, const char *filename);

static int post_file(int sockfd, newspost_data *data, file_entry *file_data,
	int filenumber, int number_of_files, const char *filestring,
	char *data_buffer);

static Buff *make_subject(Buff *subject, newspost_data *data,
	int filenumber, int number_of_files, const char *filename,
	int partnumber, int number_of_parts, const char *filestring);

/**
*** Public Routines
**/

int newspost(newspost_data *data, SList *file_list) {
	int retval;
	SList *parfiles = NULL;

	/* preprocess */
	if (data->text == FALSE)
		parfiles = preprocess(data, file_list);

	/* and post! */
	ui_post_start(data, file_list, parfiles);

	if (data->text == TRUE)
		retval = post_text_file(data, file_list);
	else
		retval = encode_and_post(data, file_list, parfiles);

	return retval;
}


/**
*** Private Routines
**/

static int post_text_file(newspost_data *data, SList *file_list) {
	file_entry *file_data = NULL;
	int retval = NORMAL;
	int sockfd = -1;
	Buff *text_buffer = NULL;

	/* create the socket */
	ui_socket_connect_start(data->address->data);
	sockfd = socket_create(data->address->data, data->port);
	retval = sockfd;
	if (retval < 0)
		return retval;

	ui_socket_connect_done();

	/* log on to the server */
	ui_nntp_logon_start(data->address->data);
	if (nntp_logon(sockfd, data) == FALSE) {
		socket_close(sockfd);
		return LOGON_FAILED;
	}
	ui_nntp_logon_done();

	file_data = file_list->data;
	/* post */
	text_buffer = read_text_file(text_buffer, file_data->filename->data);
	if(text_buffer != NULL)
		retval = nntp_post(sockfd, data->subject->data, data, text_buffer->data,
					text_buffer->length, TRUE);

	buff_free(text_buffer);

	return retval;
}

static int encode_and_post(newspost_data *data, SList *file_list,
			    SList *parfiles) {
	int number_of_parts;
	int number_of_files;
	int i;
	file_entry *file_data = NULL;
	int retval = NORMAL;
	int sockfd = -1;
	char *data_buffer = 
		(char *) malloc(get_buffer_size_per_encoded_part(data));
	Buff *subject = NULL;
	Buff *text_buffer = NULL;

	/* create the socket */
	ui_socket_connect_start(data->address->data);
	sockfd = socket_create(data->address->data, data->port);
	retval = sockfd;
	if (retval < 0)
		return retval;

	ui_socket_connect_done();

	/* log on to the server */
	ui_nntp_logon_start(data->address->data);
	if (nntp_logon(sockfd, data) == FALSE) {
		socket_close(sockfd);
		return LOGON_FAILED;
	}
	ui_nntp_logon_done();

	/* post any sfv files... */
	if (data->sfv != NULL) {
		file_data = file_entry_alloc(file_data);
		file_data->filename =
			buff_create(file_data->filename, "%s", data->sfv->data);
		if (stat(data->sfv->data, &file_data->fileinfo) == -1)
			ui_sfv_gen_error(data->sfv->data, errno);
		else {
			retval = post_file(sockfd, data, file_data, 1, 1,
						"SFV File", data_buffer);
			if (retval < 0)
				return retval;

			unlink(data->sfv->data);
		}
		free(file_data);
	}

	number_of_files = slist_length(file_list);

	/* if there's a prefix, post that */
	if (data->prefix != NULL) {
		ui_posting_prefix_start(data->prefix->data);

		file_data = (file_entry *) file_list->data;
		number_of_parts =
			get_number_of_encoded_parts(data, file_data);
		subject = make_subject(subject, data, 1 , number_of_files,
				file_data->filename->data, 0 , number_of_parts,
				"File");

		text_buffer = read_text_file(text_buffer, data->prefix->data);
		if (text_buffer != NULL) {
			retval = nntp_post(sockfd, subject->data, data, text_buffer->data,
						text_buffer->length, TRUE);
			if (retval == POSTING_NOT_ALLOWED)
				return retval;
			else if (retval == POSTING_FAILED) {
				/* dont bother retrying...
					who knows what's in that file */
				ui_posting_prefix_failed();
				retval = NORMAL;
			}
			else if (retval == NORMAL)
				ui_posting_prefix_done();
		}
		else
			ui_posting_prefix_failed();

		buff_free(subject);
	}

	/* post the files */
	i = 1;
	while (file_list != NULL) {

		file_data = (file_entry *) file_list->data;

		retval = post_file(sockfd, data, file_data, i, number_of_files,
					"File", data_buffer);
		if (retval < 0)
			return retval;

		i++;
		file_list = slist_next(file_list);
	}

	/* post any par files */
	i = 1;
	file_list = parfiles;
	number_of_files = slist_length(parfiles);
	while (file_list != NULL) {

		file_data = (file_entry *) file_list->data;

		retval = post_file(sockfd, data, file_data, i, number_of_files,
					"PAR File", data_buffer);
		if (retval < 0)
			return retval;

		unlink(file_data->filename->data);
		buff_free(file_data->filename);
		free(file_data);
		i++;
		file_list = slist_next(file_list);
	}
	slist_free(parfiles);

	nntp_logoff(sockfd);
	socket_close(sockfd);
	
	free(data_buffer);
	buff_free(text_buffer);

	return retval;
}

static int post_file(int sockfd, newspost_data *data, file_entry *file_data,
	  int filenumber, int number_of_files,
	  const char *filestring, char *data_buffer) {
	long number_of_bytes;
	int j, retval;
	int number_of_tries = 0;
	int parts_posted = 0;
	int number_of_parts = 
		get_number_of_encoded_parts(data, file_data);
	static int total_failures = 0;
	boolean posting_started = FALSE;
	Buff * subject = NULL;

	if(file_data->parts != NULL){
		if(file_data->parts[0] == TRUE) return NORMAL;
	}

	for (j = 1; j <= number_of_parts; j++) {

		if ((file_data->parts != NULL) &&
		    (file_data->parts[j] == FALSE))
			continue;

		subject = make_subject(subject, data, filenumber, number_of_files,
			     file_data->filename->data, j, number_of_parts,
			     filestring);
		
		number_of_bytes = get_encoded_part(data, file_data, j,
						   data_buffer);
		if (posting_started == FALSE) {
			ui_posting_file_start(data, file_data, 
				      number_of_parts, number_of_bytes);
			posting_started = TRUE;
		}
	
		ui_posting_part_start(file_data, j, number_of_parts,
				      number_of_bytes);

		retval = nntp_post(sockfd, subject->data, data, data_buffer,
				   number_of_bytes, FALSE);

		if (retval == NORMAL) {
			ui_posting_part_done(file_data, j, number_of_parts,
					     number_of_bytes);
			parts_posted++;
		}
		else if (retval == POSTING_NOT_ALLOWED)
			return retval;
		else {
			if (number_of_tries < 5) {
				ui_nntp_posting_retry();
				sleep(5);
				number_of_tries++;
				continue;
			}
			else {
				total_failures++;
				if (total_failures == 5) {
					nntp_logoff(sockfd);
					socket_close(sockfd);
					ui_too_many_failures();
				}
			}
		}
		number_of_tries = 0;
	}
	buff_free(subject);

	ui_posting_file_done();
	return NORMAL;
}

static Buff *make_subject(Buff *subject, newspost_data *data, int filenumber,
			 int number_of_files, const char *filename,
			 int partnumber, int number_of_parts,
			 const char *filestring) {
	char numbuf[32];
	int numsize;

	if (data->subject != NULL)
		subject = buff_create(subject, "%s - ", data->subject->data);
	else
		subject = buff_create(subject, "");
	if (data->filenumber == TRUE){
		sprintf(numbuf, "%i", number_of_files);
		numsize = strlen(numbuf);
		subject = buff_add(subject, "%s %0*i of %i: ", filestring,
				   numsize, filenumber, number_of_files);
	}
	sprintf(numbuf, "%i", number_of_parts);
	numsize = strlen(numbuf);
	subject = buff_add(subject, (data->yenc == TRUE) ? "\"%s\" yEnc (%0*i/%i)" :
		"%s (%0*i/%i)", n_basename(filename), numsize,
		partnumber, number_of_parts);
	return subject;
}

static SList *preprocess(newspost_data *data, SList *file_list) {
	Buff *tmpstring = NULL;
	SList *parfiles = NULL;

	/* make the from line */
	if (data->name != NULL) {
		tmpstring = buff_create(tmpstring, "%s", data->from->data);
		data->from = buff_create(data->from, "%s <%s>",
					 data->name->data, tmpstring->data);
		buff_free(tmpstring);
	}

	/* calculate CRCs if needed; generate any sfv files */
	if ((data->yenc == TRUE) || (data->sfv != NULL)) {
		calculate_crcs(file_list);

		if (data->sfv != NULL)
			newsfv(file_list, data);
	}

	/* generate any par files */
	if (data->par != NULL) {
		parfiles = par_newspost_interface(data, file_list);
		if (data->yenc == TRUE)
			calculate_crcs(parfiles);
	}

	return parfiles;
}

/* returns number of bytes read */
static Buff *read_text_file(Buff *text_buffer, const char *filename) {
	FILE *file;
	Buff *line = NULL;

	buff_free(text_buffer);
	file = fopen(filename, "r");
	if (file != NULL) {
		while (!feof(file)) {
			line = buff_getline(line, file);
			if(line == NULL){
				text_buffer = buff_add(text_buffer, "\r\n");
				continue;
			}

			/* translate for posting */
			if (line->data[0] == '.')
				text_buffer = buff_add(text_buffer, ".");

			text_buffer = buff_add(text_buffer, "%s", line->data);

			if(text_buffer->data[(text_buffer->length - 1)] == '\r')
				text_buffer = buff_add(text_buffer, "\n");
			else
				text_buffer = buff_add(text_buffer, "\r\n");
		}
		fclose(file);
	}
	return text_buffer;
}
