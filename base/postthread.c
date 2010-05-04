/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Authors: Pietje Bell <pietjebell@pietjebell.org>
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

#include "postthread.h"
#include "queue.h"
#include "../ui/ui.h"
#include "socket.h"
#include "nntp.h"
#include "encode.h"

newspost_threadinfo_t *threadinfo_array;
pthread_key_t key_thread_id;
int nthreads;

void *poster_thread(void *arg)
{
	/* readability */
	newspost_postthreadarg_t * arguments = (newspost_postthreadarg_t *) arg;

	newspost_data *data = arguments->data;
	queue *fifo = arguments->fifo;
	int thread_id = arguments->thread_id;

	/* variable declaration/definition */
	int sockfd = -1;

	post_article_t article;
	char *data_buffer = (char *) malloc(get_buffer_size_per_encoded_part(data));

	int total_failures = 0;
	int number_of_tries = 0;

	int retval;

	int number_of_bytes;
	int number_of_parts;

	/* initialize */
	article.file_data = NULL;
	article.partnumber = -1;
	article.subject = NULL;

	/* set the thread id */
	pthread_setspecific(key_thread_id, &thread_id) ;

	/* create the socket */
	while (sockfd < 0) {
		ui_socket_connect_start(data->address->data);
		sockfd = socket_create(data->address->data, data->port);

		if (sockfd >= 0)
			break;

		ui_socket_connect_failed(sockfd);
		number_of_tries++;

		if (number_of_tries >= 5) {
			ui_too_many_failures();
			free(data_buffer);
			pthread_exit(NULL);
		}
		sleep(120);
	}
	ui_socket_connect_done();
	number_of_tries = 0;

	/* log on to the server */
	ui_nntp_logon_start(data->address->data);
	if (nntp_logon(sockfd, data) == FALSE) {
		socket_close(sockfd);
		free(data_buffer);
		pthread_exit(NULL);
	}
	ui_nntp_logon_done();

	while (TRUE) {
		pthread_mutex_lock(fifo->mut);
		while (fifo->empty && !fifo->producer_done)
			pthread_cond_wait(fifo->notEmpty, fifo->mut);

		retval = queue_item_del(fifo, &article);

		pthread_mutex_unlock(fifo->mut);

		if (retval == QUEUE_PRODUCER_DONE)
			break;

		pthread_cond_signal(fifo->notFull);

		number_of_bytes = get_encoded_part(data, article.file_data, article.partnumber, data_buffer);
		/* FIXME Recalculated for every part */
		number_of_parts = get_number_of_encoded_parts(data, article.file_data);

		ui_posting_part_start(article.file_data, article.partnumber, number_of_parts,
					number_of_bytes);


		retval = nntp_post(sockfd, article.subject->data, data, data_buffer, number_of_bytes, FALSE);

		if (retval == NORMAL) {
			ui_posting_part_done(article.file_data, article.partnumber, number_of_parts,
					number_of_bytes);
		}
		else if (retval == POSTING_NOT_ALLOWED)
			return NULL;
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

	nntp_logoff(sockfd);
	socket_close(sockfd);

	buff_free(article.subject);
	free(data_buffer);

	pthread_exit(NULL);
	return NULL;
}
