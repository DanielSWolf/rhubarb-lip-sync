/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2000                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  October 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  Some generic socket client/server code                               */
/*                                                                       */
/*************************************************************************/
#ifdef CST_NO_SOCKETS

int cst_socket_open(const char *host, int port)
{
    (void)host;
    (void)port;
    return -1;
}

int cst_socket_server(const char *name, int port,
		      int (process_client)(int name,int fd))
{
    (void)name;
    (void)port;
    (void)process_client;
    return -1;
}

int cst_socket_close(int socket)
{
    return -1;
}
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cst_socket.h"
#include "cst_error.h"

int cst_socket_open(const char *host, int port)
{   
    /* Return an FD to a remote server */
    struct sockaddr_in serv_addr;
    struct hostent *serverhost;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd < 0)  
    {
	cst_errmsg("cst_socket: can't get socket\n");
	return -1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    if ((serv_addr.sin_addr.s_addr = inet_addr(host)) == -1)
    {
	/* its a name rather than an ipnum */
	serverhost = gethostbyname(host);
	if (serverhost == (struct hostent *)0)
	{
	    cst_errmsg("cst_socket: gethostbyname failed\n");
	    return -1;
	}
	memmove(&serv_addr.sin_addr,serverhost->h_addr, serverhost->h_length);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
	cst_errmsg("cst_socket: connect to server failed\n");
	return -1;
    }

    return fd;
}

int cst_socket_close(int socket)
{
    return close(socket);
}

int cst_socket_server(const char *name, int port,
		      int (process_client)(int name,int fd))
{
    struct sockaddr_in serv_addr;
    int fd, fd1;
    int client_name = 0;
    int one = 1;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd < 0)
    {
	cst_errmsg("can't open socket %d\n",port);
	return -1;
    }

    if (setsockopt(fd, SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(int)) < 0) 
    {
	cst_errmsg("socket SO_REUSERADDR failed\n");
	return -1;
     }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
	cst_errmsg("socket: bind failed\n");
	return -1;
    }
    
    if (listen(fd, 5) != 0)
    {
	cst_errmsg("socket: listen failed\n");
	return -1;
    }

    if (name)
	printf("server (%s) started on port %d\n",name, port);

    while(1)                          /* never exits except by signals */
    {
	if((fd1 = accept(fd, 0, 0)) < 0)
	{
	    cst_errmsg("socket: accept failed\n");
	    return -1;
	}

	client_name++;

	(process_client)(client_name,fd1);

	close(fd1);
    }

    return 0;
}

#endif
