/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "newspost.h"

int socket_create(const char *address, int port);
void socket_close(int sockfd);
long socket_getline(int sockfd, char *buffer);
long socket_write(int sockfd, const char *buffer, long length);

#endif /* __SOCKET_H__ */
