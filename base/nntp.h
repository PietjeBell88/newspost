/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __NNTP_H__
#define __NNTP_H__

#include "newspost.h"
#include "queue.h"

#define NNTP_AUTHENTICATION_SUCCESSFUL "281"
#define NNTP_MORE_AUTHENTICATION_REQUIRED "381"
#define NNTP_UNKNOWN_COMMAND "500"
#define NTTP_AUTHENTICATION_UNSUCCESSFUL "502"
#define NNTP_PROCEED_WITH_POST "340"
#define NNTP_POSTING_NOT_ALLOWED "440"
#define NNTP_ARTICLE_POSTED_OK "240"
#define NNTP_POSTING_FAILED "441"
#define NNTP_DATE "111"

boolean nntp_logon(newspost_threadinfo *tinfo, newspost_data *data);
void nntp_logoff(newspost_threadinfo *tinfo);
int nntp_issue_command(newspost_threadinfo *tinfo, const char *command);
int nntp_get_response(newspost_threadinfo *tinfo, char *response);
int nntp_post(newspost_threadinfo *tinfo, const char *subject, newspost_data *data,
	      const char *buffer, long length, boolean no_ui_updates);

#endif /* __NNTP_H__ */
