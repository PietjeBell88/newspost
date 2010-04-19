/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "newspost.h"

extern int sockfd;

int socket_create(const char *address, int port);
void socket_close();
long socket_getline(char *buffer);
long socket_write(const char *buffer, long length);

#endif /* __SOCKET_H__ */
