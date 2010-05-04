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

#include <pthread.h>
#include "ui.h"
#include "errors.h"
#include "../base/encode.h"
#include "../base/postthread.h"

time_t post_delay = SLEEP_TIME;
boolean verbosity = FALSE;
Buff * tmpdir_ptr = NULL;

static time_t start_time;

pthread_mutex_t *progress_lock;
static int total_parts_posted = 0;

//static long total_bytes_posted = 0;
//static time_t total_posting_time = 0;

static int files_posted = 0;

static const char *byte_print(long numbytes);
//static void rate_print();
static void time_print(time_t interval);
static const char *plural(long i);

void ui_tmpdir_create_failed(const char *dirname, int error) {
	fprintf(stderr,
		"\nERROR: Error while creating temporary directory %s: %s\n",
		dirname, strerror(error));
}

void ui_sfv_gen_start() {
	printf("\nGenerating SFV file...");
	fflush(stdout);
}

void ui_sfv_gen_done(const char *filename) {
	printf(" %s", n_basename(filename));
	fflush(stdout);
}

void ui_sfv_gen_error(const char *filename, int error) {
	fprintf(stderr,
		"\nWARNING: Error while creating sfv file: %s %s",
			(error == 0) ? strerror(error) : "", filename);
}

void ui_crc_start() {
	printf("\nCalculating CRCs...");
	fflush(stdout);
}

void ui_crc_done() {
	printf(" done");
	fflush(stdout);
}

void ui_crc_error(const char *filename, int error) {
	fprintf(stderr,
		"\nWARNING: Error while calculating CRC: %s %s",
			(error == 0) ? strerror(error) : "", filename);
}

void ui_par_gen_start() {
	printf("\nAdding files to PAR archive...");
	fflush(stdout);	
}

void ui_par_gen_error() {
	fprintf(stderr,
		"\nWARNING: Error while creating PAR files");
}

void ui_par_file_add_done(const char *filename) {
	printf(" %s", n_basename(filename));
	fflush(stdout);
}

void ui_par_volume_create_start() {
	printf("\nCreating PAR volumes...");
	fflush(stdout);
}

void ui_par_volume_created(const char *filename) {
	printf(" %s", n_basename(filename));
	fflush(stdout);
}

void ui_post_start(newspost_data *data, SList *file_list, SList *parfiles) {
	int i, j, numparts;
	time_t waketime;
	SList *listptr = NULL;
	long total_bytes = 0;
	long file_bytes = 0;
	long par_bytes = 0;
	int partsof = 0;
	struct stat statbuf;
	file_entry *fileinfo;
	Buff * buff = NULL;

	progress_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(progress_lock, NULL);

	printf("\n");
	printf("\nFrom: %s", data->from->data);
	printf("\nNewsgroups: %s", data->newsgroup->data);

	if (data->replyto != NULL)
		printf("\nReply-To: %s", data->replyto->data);
	if (data->followupto != NULL)
		printf("\nFollowup-To: %s", data->followupto->data);
	if (data->organization != NULL)
		printf("\nOrganization: %s", data->organization->data);
	if (data->reference != NULL)
		printf("\nReferences: %s", data->reference->data);
	listptr = data->extra_headers;
	while (listptr != NULL) {
		buff = (Buff *) listptr->data;
		printf("\n%s", (char *) buff->data);
		listptr = slist_next(listptr);
	}
	if (data->subject != NULL)
		printf("\nSubject: %s", data->subject->data);

	printf("\n");

	/* what are we posting? */

	i = 0;
	listptr = file_list;
	while (listptr != NULL) {
		fileinfo = (file_entry *) listptr->data;
		if (fileinfo->parts != NULL) {
			if(fileinfo->parts[0] == FALSE){
				numparts = get_number_of_encoded_parts(data, fileinfo);
				partsof++;
				for (j = 1; j <= numparts; j++) {
					if (fileinfo->parts[j] == TRUE)
						file_bytes +=
							(fileinfo->fileinfo.st_size / numparts);
				}
			}
			else{
				i--;
			}
		}
		else
			file_bytes += fileinfo->fileinfo.st_size;

		listptr = slist_next(listptr);
		i++;
	}
	total_bytes += file_bytes;

	if (data->text == FALSE) {
		printf("\n%i File%s: ", i, plural(i));
		
		if (partsof != 0)
			printf("about %s (Only posting part of"
				" %i of these files)",
				byte_print(file_bytes), partsof);
		else
			printf("%s", byte_print(file_bytes));
		
		if (data->sfv != NULL) {
			if (stat(data->sfv->data, &statbuf) == 0) {
				printf("\n1 SFV File: %s",
				       byte_print(statbuf.st_size)); 
				total_bytes += statbuf.st_size;
			}
		}

		i = 0;
		listptr = parfiles;
		while (listptr != NULL) {
			fileinfo = (file_entry *) listptr->data;
			par_bytes += fileinfo->fileinfo.st_size;
			listptr = slist_next(listptr);
			i++;
		}

		if (i > 0) {
			printf("\n%i PAR File%s: %s", 
			       i, plural(i), byte_print(par_bytes) );
			total_bytes += par_bytes;
		}
		
		printf("\n%s %s total and posting to %s\n\n",
		       (data->yenc == TRUE) ? "Yencoding" : "UUencoding",
		       byte_print(total_bytes), data->address->data);
	}
	else {
		fileinfo = file_list->data;
		printf("\nPosting %s as text to %s\n\n", 
		       n_basename(fileinfo->filename->data), data->address->data);
	}

	/* pause before posting */

	waketime = time(NULL) + post_delay;
	while (time(NULL) < waketime) {
	
		printf("\rPosting in ");
		time_print(waketime - time(NULL));
		printf("...             ");

		fflush(stdout);
		sleep(1);
	}
	start_time = time(NULL);
}

void ui_socket_connect_start(const char *servername) {
	if (verbosity == TRUE) {
		printf("\n(Thread %d) Connecting to %s...", *((int *) pthread_getspecific(key_thread_id)), servername);
		fflush(stdout);
	}
}

void ui_socket_connect_failed(int retval) {
	if (verbosity == TRUE) {
		if (retval == FAILED_TO_RESOLVE_HOST)
			printf("\n(Thread %d) Connecting failed: \"Failed to resolve host\", retrying in 120 seconds...", *((int *) pthread_getspecific(key_thread_id)));
		else if (retval == FAILED_TO_CREATE_SOCKET)
			printf("\n(Thread %d) Connecting failed: \"Failed to create socket\", retrying in 120 seconds...", *((int *) pthread_getspecific(key_thread_id)));
		else
			printf("\n(Thread %d) Connecting failed: \"Unknown error\", retrying in 120 seconds...", *((int *) pthread_getspecific(key_thread_id)));
		fflush(stdout);
	}
}

void ui_socket_connect_done() {
	if (verbosity == TRUE) {
		printf("\n(Thread %d) Connecting done.", *((int *) pthread_getspecific(key_thread_id)));
		fflush(stdout);
	}
}

void ui_nntp_logon_start(const char *servername) {
	if (verbosity == TRUE) {
		printf("\n(Thread %d) Logging on to %s...", *((int *) pthread_getspecific(key_thread_id)), servername);
		fflush(stdout);
	}
}

void ui_nntp_logon_done() {
	if (verbosity == TRUE) {
		printf("\n(Thread %d) Logon done.\n", *((int *) pthread_getspecific(key_thread_id)));
		fflush(stdout);
	}
}

/* only called when we get 502: authentication rejected */
void ui_nntp_authentication_failed(const char *response) {
	fprintf(stderr,
		"\n(Thread %d) ERROR: NNTP authentication failed: %s", *((int *) pthread_getspecific(key_thread_id)), response);
}


/* called with EVERY command */
void ui_nntp_command_issued(const char *command) {
	/*
	printf("\n(Thread %d) command: %s", *((int *) pthread_getspecific(key_thread_id)), command);
	fflush(stdout);
	*/
}

/* called with EVERY response */
void ui_nntp_server_response(const char *response) {
	/*
	printf("\n(Thread %d) response: %s", *((int *) pthread_getspecific(key_thread_id)), response);
	fflush(stdout);
	*/
}


/* called when we get an unexpected response */
void ui_nntp_unknown_response(const char *response) {
	fprintf(stderr,
		"\n(Thread %d) WARNING: unexpected server response: %s", *((int *) pthread_getspecific(key_thread_id)), response);
}

int ui_chunk_posted(long chunksize, long bytes_written) {
	int tid = *((int *) pthread_getspecific(key_thread_id));
	newspost_threadinfo_t * tinfo = &threadinfo_array[tid];

	if (chunksize != 0) {
		pthread_mutex_lock(rwlock);
		tinfo->bytes_written += chunksize;
		pthread_mutex_unlock(rwlock);
	}
	return chunksize;
}

void ui_posting_part_start(file_entry *filedata, int part_number, 
			   int number_of_parts, long number_of_bytes) {
	//int tid = *((int *) pthread_getspecific(key_thread_id));
	//newspost_threadinfo_t * tinfo = &threadinfo_array[tid];

	/*
	pthread_mutex_lock(rwlock);
	tinfo->part_start_time = time(NULL);
	pthread_mutex_unlock(rwlock);*/
}

void ui_posting_part_done(file_entry *filedata, int part_number, 
			  int number_of_parts, long number_of_bytes) {

	pthread_mutex_lock(progress_lock);
	total_parts_posted += 1;
	pthread_mutex_unlock(progress_lock);
}

/* when we fail to post an article */
void ui_nntp_posting_failed(const char *response) {
	fprintf(stderr,
		"\nWARNING: Posting failed: %s", response);
}

void ui_nntp_posting_retry() {
	fprintf(stderr,
		"Retrying");
}

void ui_posting_file_start(newspost_data *data, file_entry *filedata, 
			   int number_of_parts, long bytes_in_first_part) {
	Buff *tmpbuff = NULL;		   

	printf("\n%s - %s (%i part%s",
	       n_basename(filedata->filename->data),
	       byte_print(filedata->fileinfo.st_size),
	       number_of_parts,
	       plural(number_of_parts));

	if (filedata->parts == NULL) {
		long estimate;

		printf(", ");

		if (data->yenc != TRUE)
			estimate = filedata->fileinfo.st_size 
				   * UU_CHARACTERS_PER_LINE
				   / BYTES_PER_LINE;
		else
			if (number_of_parts == 1)
				estimate = bytes_in_first_part;
			else {
				/* we have to guesstimate */
				estimate = bytes_in_first_part *
					(number_of_parts - 1) +
					/* ratio of last/normal partsize */
					(( (double)(filedata->fileinfo.st_size 
					% (BYTES_PER_LINE * data->lines))
					/ (BYTES_PER_LINE * data->lines))
					/* and multiply by (normal partsize) */
					* bytes_in_first_part);

				printf("about ");
			}

		printf("%s encoded)", byte_print(estimate));

		if (strlen(n_basename(filedata->filename->data)) < 20)
			printf("          ");
	}
	else {
		int i;
		int p = 0;
		boolean already = FALSE;
		boolean inrange = FALSE;

		for (i = 1; i <= number_of_parts; i++) {
			if (filedata->parts[i] == TRUE) {
				p++;
				if (inrange == FALSE) {
					if ((i > 1) &&
					    (filedata->parts[i - 1] == TRUE))
						inrange = TRUE;
					else {
						if (already == TRUE)
							tmpbuff = buff_add(tmpbuff,",");
						else
							already = TRUE;
						tmpbuff = buff_add(tmpbuff," %i", i);
					}
				}
			}
			else
				if (inrange == TRUE) {
					tmpbuff = buff_add(tmpbuff,"-%i", (i - 1));
					inrange = FALSE;
				}
		}
		
		if (inrange == TRUE)
			tmpbuff = buff_add(tmpbuff,"-%i", number_of_parts);
			
		printf(") - only posting part%s%s",plural(p),tmpbuff->data);
		tmpbuff = buff_free(tmpbuff);
	}

	printf("\n");
	fflush(stdout);
}

void ui_posting_file_done() {
	files_posted++;
}

void ui_post_done() {
	time_t totaltime = (time(NULL) - start_time);

	/*
	if(total_posting_time > 0){
		totalbps = (total_bytes_posted / total_posting_time);
	}
	else{
		totalbps = 0;
	}

	printf("\rPosted %i file%s (%s encoded) in ",
		files_posted, plural(files_posted),
		byte_print(total_bytes_posted));
*/
	time_print(totaltime);
	printf(".     ");
/*
	if(totalbps > 0){
		rate_print(totalbps);
	}
*/
	printf("\n");
}

void ui_generic_error(int error) {
	if (error != 0)
		fprintf(stderr,
			"\nWARNING: %s", strerror(error));
}

void ui_too_many_failures() {
	fprintf(stderr, "\nToo many failures."
			"\nGiving up.\n");
}

void ui_socket_error(int error){
	Buff *command = NULL;

	fprintf(stderr, "\nSocket error: %s",strerror(error));
	if (tmpdir_ptr != NULL) {
		fprintf(stderr, "\nDeleting temporary files... ");
		if (rmdir(tmpdir_ptr->data) == -1) {
			command = buff_create(command,
				 "rm -rf %s", tmpdir_ptr->data);
			system(command->data);
		}
		fprintf(stderr, "done.");
	}
	fprintf(stderr,"\n");

	exit(EXIT_SOCKET_ERROR);
}

/**
*** Private Routines
**/

static const char *byte_print(long numbytes) {
	static char byte_string[64];

	if (numbytes < 1024)
		sprintf(byte_string, "%li byte%s", numbytes,
			 plural(numbytes));
	else if (numbytes < (1024 * 1024))
		sprintf(byte_string, "%li KB", numbytes / 1024);
	else
		sprintf(byte_string, "%.1f MB",
			(double) numbytes / (1024 * 1024));

	return byte_string;
}

void rate_print(int sleepytime) {
	long bytes_written = 0;
	long bps;
	int i;
	newspost_threadinfo_t * tinfo = NULL;

	pthread_mutex_lock(rwlock);
	for (i = 0; i < nthreads; i++) {
		tinfo = &threadinfo_array[i];
		bytes_written += tinfo->bytes_written;
		tinfo->bytes_written = 0;

	}
	pthread_mutex_unlock(rwlock);

	bps = bytes_written / sleepytime;

	if (bps > 1048576)
		printf("%lf MB/second\r", (double) bps / 1048576);
	else if (bps > 1024)
		printf("%1f KB/second\r", (double) bps / 1024);
	else
		printf("%li bytes/second\r", bps);
	fflush(stdout);
}

static void time_print(time_t interval) {
	if (interval > 3600) {
		long hours = interval / 3600;
		printf("%li hour%s ", hours, plural(hours));
		interval = interval % 3600;
	}
	if (interval > 60) {
		long minutes = interval / 60;
		printf("%li minute%s ", minutes, plural(minutes));
		interval = interval % 60;
	}
	printf("%li second%s", (long) interval,
		plural((long) interval));
}

static const char *plural(long i) {
	return (i != 1) ? "s" : "";
}
