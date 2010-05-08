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

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"
#include "../ui/ui.h"

/**
*** Public Routines
**/

int socket_create(const char *address, int port) {
	struct sigaction act, oact; /* used to prevent a crash in
				       case of a dead socket */
	/* use these to make the socket to the host */
	struct sockaddr_in serv_addr;
	struct in_addr inaddr;
	int on;
	struct hostent *hp;

#if (defined(sun) || defined(__sun)) && (defined(__svr4) || \
defined(__SVR4) || defined(__svr4__))
	unsigned long tinaddr;
#endif
	int sockfd = -1;

	memset ( &serv_addr, 0, sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	/* Try to convert host name as dotted decimal */
#if (defined(sun) || defined(__sun)) && (defined(__svr4) || \
defined(__SVR4) || defined(__svr4__))
	tinaddr = inet_addr(address);
	if ( tinaddr != -1 )
#else
	if ( inet_aton(address, &inaddr) != 0 ) 
#endif
	{
		memcpy(&serv_addr.sin_addr, &inaddr, sizeof(inaddr));
	}
	else	/* If that failed, then look up the host name */
	{
		if (NULL == (hp = gethostbyname(address)))
			return FAILED_TO_RESOLVE_HOST;
		memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return FAILED_TO_CREATE_SOCKET;

	if (connect(sockfd, (struct sockaddr*) &serv_addr,
	    sizeof(serv_addr)) < 0) {
		close (sockfd);
		return FAILED_TO_CREATE_SOCKET;
	}

	on = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
	    (char*) &on, sizeof(on)) < 0) {
		close(sockfd);
		return FAILED_TO_CREATE_SOCKET;
	}

	/* If we write() to a dead socket, then let write return
	 * EPIPE rather than crashing the program. */

	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGPIPE, &act, &oact);

        return sockfd;
}

void socket_close(int sockfd) {
	if (sockfd >= 0)
		close(sockfd);
}

/* returns the number of bytes written */
long socket_write(int sockfd, const char *buffer, long length) {
	long retval;

	retval =  write(sockfd, buffer, length);
	if(retval < 0){
		ui_socket_error(errno);
		return retval; /* never happens */
	}
	else{
		return retval;
	}
}

/* returns the number of bytes read */
long socket_getline(int sockfd, char *buffer) {
	long retval;
	char *pi;
	long i;
	long read_count = 0;

	i = 0;
	pi = buffer;
	while (TRUE) {
		retval = read(sockfd, pi, 1);
		if(retval < 0)
			ui_socket_error(errno);
		read_count += retval;
		pi++;
		if (buffer[i] == '\n')
			break;
		i++;
	}
	*pi = '\0';

	return read_count;
}
