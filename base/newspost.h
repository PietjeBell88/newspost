/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __NEWSPOST_H__
#define __NEWSPOST_H__

/*********************************/
/* USER-CONFIGURABLE DEFINITIONS */
/*********************************/

#define SLEEP_TIME 10 /* time to pause before posting in seconds */

/* #define ALLOW_NO_SUBJECT */ /* makes the subject line optional */

/* #define WINSFV32_COMPATIBILITY_MODE */

/* #define REPORT_ONLY_FULLPARTS */ /* limit update of KBps display */

/* ONLY CHANGE THESE IF YOU GET AN ERROR DURING COMPILATION */
typedef unsigned char           n_uint8; /* 1 byte unsigned integer */
typedef unsigned short int      n_uint16; /* 2 byte unsigned integer */
typedef unsigned int       	n_uint32; /* 4 byte unsigned integer */
typedef long long               n_int64; /* 8 byte signed integer */

/***********************************/
/* END OF CONFIGURABLE DEFINITIONS */
/***********************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

typedef n_uint8                 boolean;
#define FALSE 0
#define TRUE 1

#include "utils.h"

/* remember to change VERSION and PAR_CLIENT for new version numbers */
#define VERSION "2.1.1"
#define PAR_CLIENT 0xFE020101

#define NEWSPOSTURL "http://newspost.unixcab.org/"
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

typedef struct {
	Buff * subject;
	Buff * newsgroup;
	Buff * from;
	Buff * organization;
	Buff * address;	/* server address and port */
	int port;
	Buff * user;
	Buff * password;
	int lines;			/* lines per message */
	boolean yenc;
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

int newspost(newspost_data *data, SList *file_list);

#endif /* __NEWSPOST_H__ */
