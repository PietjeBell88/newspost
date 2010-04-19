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

#include "../base/newspost.h"

#include <signal.h>

#include "errors.h"
#include "options.h"
#include "ui.h"

boolean writedefaults = FALSE;
boolean temporary_prefix = FALSE;

const char *EDITOR = NULL;

static void signal_handler();

int main(int argc, char **argv) {
	newspost_data main_data;
	struct sigaction act;
	int optind, retval, fd;
	FILE *fp;
	SList *file_list;
	SList *pi;
	boolean temporary_message = FALSE;
	boolean posting_anything = FALSE;
	Buff *command = NULL;
	file_entry *file_data = NULL;

	retval = NORMAL;

	if (argc == 1)
		print_help();

	/* initialize the data */
	main_data.subject = NULL;
	main_data.newsgroup = NULL;
	main_data.from = NULL;
	main_data.organization = NULL;
	main_data.address = NULL;
	main_data.port = 119;
	main_data.user = NULL;
	main_data.password = NULL;
	main_data.lines = 7500;
	main_data.prefix = NULL;
	main_data.yenc = FALSE;
	main_data.sfv = NULL;
	main_data.par = NULL;
	main_data.parnum = 0;
	main_data.filesperpar = 10;
	main_data.reference = NULL;
	main_data.filenumber = FALSE;
	main_data.tmpdir = NULL;
	main_data.noarchive = TRUE;
	main_data.followupto = NULL;
	main_data.replyto = NULL;
	main_data.name = NULL;
	main_data.extra_headers = NULL;
	main_data.text = FALSE;

	/* get all options */
	parse_environment(&main_data);
	parse_defaults(&main_data);
	optind = parse_options(argc, argv, &main_data);

	file_list = (SList *) parse_input_files(argc, argv, optind,
						&main_data);

	if (writedefaults == TRUE) {
		printf( (set_defaults(&main_data) == TRUE) ?
			"\nDefaults written." :
			"\nDefaults NOT written.");
		fflush(stdout);
	}

	check_options(&main_data);

	if ((main_data.text == FALSE) && (file_list == NULL)) {
		fprintf(stderr, "\nNo files to post!\n");
		exit(EXIT_NO_FILES);
	}

	if ((main_data.text == TRUE) && (slist_length(file_list) > 1)) {
		fprintf(stderr, "\nOnly one text file may be posted.\n");
		exit(EXIT_TOO_MANY_TEXT_FILES);
	}

	if(main_data.text == FALSE){
		if((main_data.sfv == NULL)
		   && (main_data.par == NULL)){
			pi = file_list;
			while(pi != NULL){
				file_data = (file_entry *) pi->data;
				if (file_data->parts != NULL) {
					if(file_data->parts[0] == FALSE){
						posting_anything = TRUE;
						break;
					}
				}
				else{
					posting_anything = TRUE;
					break;
				}
				
				pi = slist_next(pi);
			}
			if(posting_anything == FALSE){
				fprintf(stderr, "\nNo files to post!\n");
				exit(EXIT_NO_FILES);
			}
		}
	}

	/* create the directory for storing temporary files */
	/* only create it if we need it */
	if ((main_data.sfv != NULL)
	   || (main_data.par != NULL)
	   || (temporary_prefix == TRUE)
	   || ((main_data.text == TRUE) && (file_list == NULL)) ) {
		if (main_data.tmpdir == NULL)
			main_data.tmpdir = buff_create(main_data.tmpdir, "%s", P_tmpdir);
		main_data.tmpdir = buff_add(main_data.tmpdir, "/newspost-XXXXXX");
		if((fd = mkstemp(main_data.tmpdir->data)) == -1){
			ui_tmpdir_create_failed(main_data.tmpdir->data, errno);
			exit(EXIT_FAILED_TO_CREATE_TMPFILES);
		}
		close(fd);
		unlink(main_data.tmpdir->data);
		if (mkdir(main_data.tmpdir->data, S_IRWXU) == -1) {
			ui_tmpdir_create_failed(main_data.tmpdir->data, errno);
			exit(EXIT_FAILED_TO_CREATE_TMPFILES);
		}
		else { 
			/* we catch the signal and delete tmpdir */
			act.sa_handler = signal_handler;
			sigemptyset(&act.sa_mask);
			act.sa_flags = 0;
			sigaction(SIGHUP, &act, NULL);
			sigaction(SIGINT, &act, NULL);
			sigaction(SIGTERM, &act, NULL);
			tmpdir_ptr = main_data.tmpdir;

			if ((main_data.text == TRUE) && (file_list == NULL)) {
				file_data = file_entry_alloc(file_data);
				file_data->filename = buff_create(file_data->filename,
					 "%s/message", main_data.tmpdir->data);
				command = buff_create(command, "%s %s",
					 (EDITOR != NULL) ? EDITOR : "vi",
					 file_data->filename->data);
				system(command->data);
				
				fp = fopen(file_data->filename->data, "rb");
				if (fp != NULL) {
					fclose(fp);
					chmod(file_data->filename->data,
						S_IRUSR | S_IWUSR);
					stat(file_data->filename->data,
						&file_data->fileinfo);
					file_data->parts = NULL;
					file_list = slist_prepend(file_list,
								  file_data);
					temporary_message = TRUE;
				}
				else {
					buff_free(file_data->filename);
					free(file_data);
					if (rmdir(main_data.tmpdir->data) == -1) {
						command = buff_create(command,
							 "rm -rf %s",
							 main_data.tmpdir->data);
						system(command->data);
					}
					fprintf(stderr,
						"\nCannot open message: %s"
						"\nNothing to post!\n", 
						strerror(errno));
					exit(EXIT_NO_FILES);
				}
			}
			else if ((temporary_prefix == TRUE) &&
				 (retval == NORMAL) &&
				 (main_data.text == FALSE)) {
				main_data.prefix = buff_create(main_data.prefix,
					 "%s/prefix", main_data.tmpdir->data);

				command = buff_create(command, "%s %s",
					 (EDITOR != NULL) ? EDITOR : "vi",
					 main_data.prefix->data);
				system(command->data);
				
				fp = fopen(main_data.prefix->data, "rb");

				if (fp != NULL) {
					fclose(fp);
					chmod(main_data.prefix->data,
						S_IRUSR|S_IWUSR);
				}
				else {
					fprintf(stderr,
						"\nWARNING: Cannot open"
						" prefix: %s - SKIPPING",
						strerror(errno));
					main_data.prefix = buff_free(main_data.prefix);
				}
			}
		}
	}
	/* post */
	if (NORMAL == retval)
		retval = newspost(&main_data, file_list);

	/* free all the data */
	pi = file_list;
	while (pi != NULL) {
		file_data = (file_entry *) pi->data;
		if (temporary_message == TRUE) {
			unlink(file_data->filename->data);
			file_entry_free(file_data);
			/* if the check for length(file_list) > 1 is
			 * ever removed, not all data will be freed */
			break;
		}
		file_entry_free(file_data);
		pi = slist_next(pi);
	}
	slist_free(file_list);
	pi = main_data.extra_headers;
	while (pi != NULL) {
		buff_free(pi->data);
		pi = slist_next(pi);		
	}
	if ((TRUE == temporary_prefix) && (main_data.text == FALSE))
		unlink(main_data.prefix->data);

	/* if we forgot to remove a file in tmpdir, force its removal */
	if ((main_data.sfv != NULL)
	   || (main_data.par != NULL)
	   || (temporary_prefix == TRUE)
	   || (temporary_message == TRUE)) {	
		if (rmdir(main_data.tmpdir->data) == -1) {
			command = buff_create(command,
				 "rm -rf %s", main_data.tmpdir->data);
			system(command->data);
		}
	}
	buff_free(command);

	/* free the data */
	buff_free(main_data.subject);
	buff_free(main_data.newsgroup);
	buff_free(main_data.from);
	buff_free(main_data.organization);
	buff_free(main_data.address);
	buff_free(main_data.user);
	buff_free(main_data.password);
	buff_free(main_data.prefix);
	buff_free(main_data.sfv);
	buff_free(main_data.par);
	buff_free(main_data.reference);
	buff_free(main_data.tmpdir);
	buff_free(main_data.followupto);
	buff_free(main_data.replyto);
	buff_free(main_data.name);
	if (main_data.extra_headers != NULL)
		slist_free(main_data.extra_headers);

	switch (retval) {

	case NORMAL:
		if (main_data.text == TRUE)
			printf("\nFile posted as text.\n");
		else
			ui_post_done();
		exit(EXIT_NORMAL);

	case FAILED_TO_CREATE_TMPFILES:
		fprintf(stderr,
			"\nFailed to create temporary directory\n");
		exit(EXIT_FAILED_TO_CREATE_TMPFILES);

	case FAILED_TO_RESOLVE_HOST:
		fprintf(stderr,
			"\nFailed to resolve the server name\n");
		exit(EXIT_FAILED_TO_RESOLVE_HOST);

	case FAILED_TO_CREATE_SOCKET:
		fprintf(stderr,
			"\nFailed to connect to the server\n");
		exit(EXIT_FAILED_TO_CREATE_SOCKET);

	case LOGON_FAILED:
		fprintf(stderr,
			"\nFailed to logon to the server\n");
		exit(EXIT_LOGON_FAILED);

	case POSTING_NOT_ALLOWED:
		fprintf(stderr,
			"\nPosting is not allowed\n");
		exit(EXIT_POSTING_NOT_ALLOWED);

	default:
		fprintf(stderr,
			"\nInternal error.  "
			"Please contact the author.\n");
		exit(EXIT_UNKNOWN);
	}
}

static void signal_handler() {
	Buff *command = NULL;

	fprintf(stderr, "\nSignal received");
	if (tmpdir_ptr != NULL) {
		fprintf(stderr, "\nDeleting temporary files... ");
		if (rmdir(tmpdir_ptr->data) == -1) {
			command = buff_create(command,
				 "rm -rf %s", tmpdir_ptr->data);
			system(command->data);
		}
		fprintf(stderr, "done.");
	}
	fprintf(stderr, "\n");
	exit(EXIT_SIGNAL);
}
