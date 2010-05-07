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

#include "queue.h"
#include "encode.h"
#include "utils.h"

#include <pthread.h>

queue *queue_init(int length) {
	queue *q;
	int i;

	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	q->length = length;
	
	q->article_list = (post_article_t *) malloc(q->length * sizeof(post_article_t));

	/* initialize the values */
	for (i = 0; i < q->length; i++) {
		q->article_list[i].file_data = NULL;
		q->article_list[i].partnumber = -1;
		q->article_list[i].subject = NULL;
	}

	q->empty = TRUE;
	q->full = FALSE;
	q->producer_done = FALSE;
	q->head = 0;
	q->tail = 0;
	q->mut = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(q->mut, NULL);
	q->notFull = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	pthread_cond_init(q->notFull, NULL);
	q->notEmpty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	pthread_cond_init(q->notEmpty, NULL);

	return (q);
}

void queue_delete(queue *q) {

	int i;

	for (i = 0; i < q->length; i++)
		buff_free(q->article_list[i].subject);

	free(q->article_list);

	pthread_mutex_destroy(q->mut);
	free(q->mut);
	pthread_cond_destroy(q->notFull);
	free(q->notFull);
	pthread_cond_destroy(q->notEmpty);
	free(q->notEmpty);
	free(q);
}

void queue_item_add(queue *q, post_article_t *in) {

	/* readability */
	post_article_t *item = &q->article_list[q->tail];

	/* copy the article */
	item->file_data = in->file_data;
	item->partnumber = in->partnumber;
	item->subject = buff_create(item->subject, "%s", in->subject->data);
	
	/* update queue info */
	q->tail++;
	if (q->tail == q->length)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = TRUE;
	q->empty = FALSE;

	return;
}

int queue_item_del(queue *q, post_article_t *out) {

	if (q->empty && q->producer_done)
		return QUEUE_PRODUCER_DONE;
	else if (q->empty)
		return 0;

	/* readability */
	post_article_t *item = &q->article_list[q->head];
	
	/* copy the article */
	out->file_data = item->file_data;
	out->partnumber = item->partnumber;
	out->subject = buff_create(out->subject, "%s", item->subject->data);

	/* update queue info */
	q->head++;
	if (q->head == q->length)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = TRUE;
	q->full = FALSE;

	/* calculate the length of the queue */
	int n = q->tail - q->head;
	if (n < 0)
		n += q->length;

	return n;
}
