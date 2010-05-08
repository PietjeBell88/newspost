/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __NEWSPOST_H__
#define __NEWSPOST_H__

/*********************************/
/* USER-CONFIGURABLE DEFINITIONS */
/*********************************/

#define SLEEP_TIME 10 /* time to pause before posting in seconds */

#define SOCKET_RECONNECT_WAIT_SECONDS 120 /* time to wait between connect retries */

/* #define ALLOW_NO_SUBJECT */ /* makes the subject line optional */

/* #define WINSFV32_COMPATIBILITY_MODE */

/* #define REPORT_ONLY_FULLPARTS */ /* limit update of KBps display */

/***********************************/
/* END OF CONFIGURABLE DEFINITIONS */
/***********************************/

#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

/* remember to change VERSION and PAR_CLIENT for new version numbers */
#define VERSION "2.2.1"
#define PAR_CLIENT 0xFE020101

#define NEWSPOSTURL "http://github.com/PietjeBell88/newspost"
#define NEWSPOSTNAME "Newspost"
#define USER_AGENT NEWSPOSTNAME "/" VERSION " (" NEWSPOSTURL ")"

#define STRING_BUFSIZE 1024

#define NORMAL 0
#define FAILED_TO_CREATE_TMPFILES -1
#define FAILED_TO_RESOLVE_HOST -2
#define FAILED_TO_CREATE_SOCKET -3
#define LOGON_FAILED -4
#define POSTING_NOT_ALLOWED -5
#define POSTING_FAILED -6

#define THREAD_INITIALIZING 0
#define THREAD_CONNECTING 1
#define THREAD_WAITING 2
#define THREAD_POSTING 3
#define THREAD_DONE 4

typedef struct {
	Buff * subject;
	Buff * newsgroup;
	Buff * from;
	Buff * organization;
	Buff * address;	/* server address and port */
	int port;
	Buff * user;
	Buff * password;
	int threads;
	int lines;			/* lines per message */
	boolean uuenc;
	Buff * sfv;	/* filename for generated sfv file */
	Buff * par;	/* prefix filename for par file(s) */
	int parnum;			/* number of pars */
	int filesperpar;		/* or number of files per par */
	Buff * reference; /* message-id to reference */
	boolean filenumber;		/* include "File X of Y" in subject? */
	Buff * tmpdir;
	boolean noarchive;		/* include X-No-Archive: yes header? */
	Buff * followupto;
	Buff * replyto;
	Buff * name;
	boolean text;
	SList * extra_headers;
}
newspost_data;

typedef struct {
	int thread_id;
	int sockfd;

	/* only the following properties need locking */
	pthread_rwlock_t *rwlock;
	int status;
	long bytes_written; /* since the last progress */
}
newspost_threadinfo;

int newspost(newspost_data *data, SList *file_list);

#endif /* __NEWSPOST_H__ */
