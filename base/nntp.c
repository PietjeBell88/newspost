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

#include "nntp.h"
#include "../ui/ui.h"
#include "socket.h"

/**
*** Public Routines
**/

boolean nntp_logon(newspost_threadinfo *tinfo, newspost_data *data) {
	char buffer[STRING_BUFSIZE];

	if (nntp_get_response(tinfo, buffer) < 0)
		return FALSE;

	if (data->user != NULL) {
		sprintf(buffer, "AUTHINFO USER %s", data->user->data);
		if (nntp_issue_command(tinfo, buffer) < 0)
			return FALSE;
		if (nntp_get_response(tinfo, buffer) < 0)
			return FALSE;
		/* 381: More Authentication required */
		if (strncmp(buffer,
		    NNTP_MORE_AUTHENTICATION_REQUIRED, 3) == 0) {
			sprintf(buffer, "AUTHINFO PASS %s", data->password->data);
			if (nntp_issue_command(tinfo, buffer) < 0)
				return FALSE;
			if (nntp_get_response(tinfo, buffer) < 0)
				return FALSE;
			if (strncmp(buffer,
			    NTTP_AUTHENTICATION_UNSUCCESSFUL, 3) == 0) {
				ui_nntp_authentication_failed(tinfo, buffer);
				return FALSE;
			}
		}
		/* 281: Authentication successful */
		else if (strncmp(buffer,
			 NNTP_AUTHENTICATION_SUCCESSFUL, 3) == 0) {
			return TRUE;
		}
		/* 500: server doesn't support authinfo */
		else if (strncmp(buffer, NNTP_UNKNOWN_COMMAND, 3) == 0) {
			/* ignore it */
		}
		/* unknown response */
		else
			ui_nntp_unknown_response(tinfo, buffer);
	}

	return TRUE;
}

void nntp_logoff(newspost_threadinfo *tinfo) {
	char tmpbuffer[STRING_BUFSIZE];
	nntp_issue_command(tinfo, "QUIT");
	nntp_get_response(tinfo, tmpbuffer);
}

int nntp_post(newspost_threadinfo *tinfo, const char *subject, newspost_data *data,
	      const char *buffer, long length,
	      boolean no_ui_updates) {
	char response[STRING_BUFSIZE];
	const char *pi;
	long i, chunksize;
	SList * listptr;
	Buff * buff = NULL;
	Buff * tmpbuff = NULL;
	int sockfd = tinfo->sockfd;

	nntp_issue_command(tinfo, "POST");

	nntp_get_response(tinfo, response);

	if (strncmp(response, NNTP_POSTING_NOT_ALLOWED, 3) == 0)
		return POSTING_NOT_ALLOWED;

	if (strncmp(response, NNTP_PROCEED_WITH_POST, 3) != 0) {
		/* this shouldn't really happen */
		ui_nntp_unknown_response(tinfo, response);
		return POSTING_FAILED;
	}
	
	buff = buff_add(buff, "From: %s\r\n", data->from->data);
	buff = buff_add(buff, "Newsgroups: %s\r\n", data->newsgroup->data);
	buff = buff_add(buff, "Subject: %s\r\n", subject);
	buff = buff_add(buff, "User-Agent: %s\r\n", USER_AGENT);

	if (data->replyto != NULL) {
		buff = buff_add(buff, "Reply-To: %s\r\n", data->replyto->data);
	}
	if (data->followupto != NULL) {
		buff = buff_add(buff, "Followup-To: %s\r\n", data->followupto->data);
	}
	if (data->organization != NULL) {
		buff = buff_add(buff,"Organization: %s\r\n",data->organization->data);
	}
	if (data->reference != NULL) {
		buff = buff_add(buff, "References: %s\r\n", data->reference->data);
	}
	if (data->noarchive == TRUE)
		buff = buff_add(buff, "X-No-Archive: yes\r\n");
	listptr = data->extra_headers;
	while (listptr != NULL) {
		tmpbuff = (Buff *) listptr->data;
		buff = buff_add(buff, "%s\r\n", tmpbuff->data);
		listptr = slist_next(listptr);
	}
	buff = buff_add(buff,"\r\n");

	socket_write(sockfd, buff->data, buff->length);

	if (!no_ui_updates)
		ui_chunk_posted(tinfo, 0, 0);

	pi = buffer;
	i = 0;
	chunksize = 32768;
	while ((length - i) > chunksize) {
		socket_write(sockfd, pi, chunksize);
		i += chunksize;
		pi += chunksize;
		pthread_rwlock_wrlock(tinfo->rwlock);
		tinfo->bytes_written += chunksize;
		pthread_rwlock_unlock(tinfo->rwlock);
#ifndef REPORT_ONLY_FULLPARTS
		if (!no_ui_updates)
			chunksize = ui_chunk_posted(tinfo, chunksize, i);
#endif
	}
	socket_write(sockfd, pi, (length - i));
	i += (length - i);

	socket_write(sockfd, "\r\n.\r\n", 5);

	nntp_get_response(tinfo, response);

	pthread_rwlock_wrlock(tinfo->rwlock);
	tinfo->bytes_written += (length - i);
	pthread_rwlock_unlock(tinfo->rwlock);

	if (!no_ui_updates)
		ui_chunk_posted(tinfo, (length - i), i);

	if (strncmp(response, NNTP_POSTING_FAILED, 3) == 0) {
		ui_nntp_posting_failed(tinfo, response);
		return POSTING_FAILED;
	}
	else if (strncmp(response, NNTP_ARTICLE_POSTED_OK, 3) != 0) {
		/* shouldn't really happen */
		ui_nntp_unknown_response(tinfo, response);
		return POSTING_FAILED;
	}
	buff_free(buff);
	return NORMAL;
}

/* returns number of bytes written */
int nntp_issue_command(newspost_threadinfo *tinfo, const char *command) {
	int bytes_written;
	int sockfd = tinfo->sockfd;

	bytes_written = socket_write(sockfd, command, strlen(command));
	if (bytes_written > 0) {
		bytes_written += socket_write(sockfd, "\r\n", 2);
		ui_nntp_command_issued(tinfo, command);
	}
	return bytes_written;
}

/* returns number of bytes read */
int nntp_get_response(newspost_threadinfo *tinfo, char *response) {
	int bytes_read;

	bytes_read = socket_getline(tinfo->sockfd, response);
	if (bytes_read > 0)
		ui_nntp_server_response(tinfo, response);

	return bytes_read;
}
