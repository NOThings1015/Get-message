/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  socket.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(11/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "11/04/25 17:00:19"
 *                 
 ********************************************************************************/
#ifndef SOCKET_H_

#define SOCKET_H_

#define HOSTNAME_LEN          64

typedef struct socket_ctx_s
{
	char        host[HOSTNAME_LEN]; /*  CLIENT: Connect server hostname; SERVER: Unused */
	int         port;               /*  CLIENT: Connect server port;     SERVER: listen port */
	int         fd;                 /*  socket descriptor  */
	int         connected;          /*  socket connect status: 1->connected 0->disconnected  */
} socket_ctx_t;


extern int socket_server_init(char *listen_ip,int listen_port); //服务器端定义

extern int socket_init(socket_ctx_t *sock, char *host, int port); 

extern int socket_connect(socket_ctx_t *sock);

extern int socket_status(socket_ctx_t *sock);

extern int socket_terminate(socket_ctx_t *sock);

extern int socket_send(socket_ctx_t *sock, char *data, int length);

#endif

