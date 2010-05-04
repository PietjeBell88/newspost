/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __NEWSPOST_POSTTHREAD_H__
#define __NEWSPOST_POSTTHREAD_H__

#include <pthread.h>
#include <time.h>
#include "newspost.h"
#include "queue.h"

typedef struct {
	newspost_data *data;
	queue *fifo;
	int thread_id;
} newspost_postthreadarg_t;

typedef struct {
	//pthread_mutex_t *rwlock;
	int threadid;
	long bytes_written; /* since the last progress */
} newspost_threadinfo_t;

void *poster_thread(void *arg);

pthread_mutex_t *rwlock;
extern newspost_threadinfo_t *threadinfo_array;
extern pthread_key_t key_thread_id;
extern int nthreads;

#endif /* __NEWSPOST_POSTTHREAD_H__ */
