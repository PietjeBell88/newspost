/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __NEWSPOST_QUEUE_H__
#define __NEWSPOST_QUEUE_H__

#include <pthread.h>
#include "utils.h"

#define QUEUE_PRODUCER_DONE -1

typedef struct {
	file_entry *file_data;
	int partnumber;
	Buff *subject;
} post_article_t;

typedef struct {
	post_article_t *article_list;
	long length;

	long head, tail;
	boolean full, empty, producer_done;
	pthread_mutex_t *mut;
	pthread_cond_t *cond_not_full, *cond_not_empty, *cond_producer_done, *cond_empty;
} queue;

queue *queue_init(int length);
void queue_delete(queue *q);
void queue_item_add(queue *q, post_article_t *in);
int queue_item_del(queue *q, post_article_t *out);

#endif /* __NEWSPOST_QUEUE_H__ */
