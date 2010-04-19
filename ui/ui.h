/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __UI_H__
#define __UI_H__

#include <time.h>
#include "../base/newspost.h"

extern time_t post_delay;
extern boolean verbosity;
extern Buff * tmpdir_ptr;

void ui_tmpdir_create_failed(const char *dirname, int error);

void ui_sfv_gen_start();
void ui_sfv_gen_done(const char *filename);
void ui_sfv_gen_error(const char *filename, int error);

void ui_crc_start();
void ui_crc_done();
void ui_crc_error(const char *filename, int error);

void ui_par_gen_start();
void ui_par_gen_error();
void ui_par_file_add_done(const char *filename);
void ui_par_volume_create_start();
void ui_par_volume_created(const char *filename);

void ui_post_start(newspost_data *data, SList *file_list, SList *parfiles);

void ui_socket_connect_start(const char *servername);
void ui_socket_connect_done();

void ui_nntp_logon_start(const char *servername);
void ui_nntp_logon_done();
void ui_nntp_authentication_failed(const char *response);
void ui_nntp_command_issued(const char *command);
void ui_nntp_server_response(const char *response);
void ui_nntp_unknown_response(const char *response);

void ui_posting_prefix_start(const char *filename);
void ui_posting_prefix_done();
void ui_posting_prefix_failed();
void ui_posting_file_start(newspost_data *data, file_entry *filedata, 
			   int number_of_parts, long bytes_in_first_part);
void ui_posting_file_done();

int ui_chunk_posted(long chunksize, long bytes_written);

void ui_posting_part_start(file_entry *filedata, int part_number, 
			   int number_of_parts, long number_of_bytes);

void ui_posting_part_done(file_entry *filedata, int part_number, 
			  int number_of_parts, long number_of_bytes);

void ui_nntp_posting_failed(const char *response);
void ui_nntp_posting_retry();

void ui_post_done();

void ui_generic_error(int error);

void ui_too_many_failures();

void ui_socket_error(int error);

#endif /* __UI_H__ */
