/*********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  socket.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "11/04/25 16:53:52"
 *                 
 ********************************************************************************/
#include<stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <netdb.h> 
#include <netinet/tcp.h>

#include "socket.h"
#include "logger.h"


int socket_server_init(char *listen_ip,int listen_port)
{
	struct sockaddr_in          servaddr;
	int                         rv=0;
	int                         on=1;
	int                         listenfd;

	if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{   
		printf("Use socket() to create a TCP socket failure:%s\n",strerror(errno));
		return -1; 
	}   

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(listen_port);

	if(listen_ip == 0)
	{   
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //if don't set IP,Please listen all of it;
	}   

	else
	{   
		if(inet_pton(AF_INET, listen_ip, &servaddr.sin_addr) <=0)
		{   
			printf("inet_pton() set listen IP address failure:%s\n",strerror(errno));
			rv = -2; 
			close(listenfd);
		}   
	}   

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{   
		printf("Use bind() to  bind the TCP socket failure:%s\n",strerror(errno));
		rv = -3; 
		close(listenfd);
	}   

	if(listen(listenfd, 200) < 0)
	{   
		printf("Use listen() to listen listenfd failure:%s",strerror(errno));
		rv = -4; 
		close(listenfd);
	}   

	rv = listenfd;
	return rv; 

}



int socket_init(socket_ctx_t *sock, char *host, int port)
{
	if( !sock || port <= 0 )
	{
		return -1;
	}

	memset(sock, 0, sizeof(*sock));
	sock->fd = -1;
	sock->port = port;
	if( host )
	{
		strncpy(sock->host, host, HOSTNAME_LEN);
	}

	sock->connected = 0;
	return 0;
}



int socket_terminate(socket_ctx_t *sock)
{
	if( !sock )
		return -1;

	if( sock->fd > 0)
	{
		close(sock->fd);
		sock->fd = -1;
		sock->connected = 0;
	}

	return 0;
}


int socket_connect(socket_ctx_t *sock)
{
	int       	            	sockfd = 0;
	int							rv = -1;
	char 						service[16];
	struct addrinfo				hints, *rp;
	struct addrinfo			   *res = NULL;	
	struct in_addr				inaddr;
	struct sockaddr_in			addr;
	int							len = sizeof(addr);
	

	if( !sock )
	{
		return -1;
	}

	socket_terminate(sock);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; 					/*  support IPv4 and IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; 			/*  TCP protocol */

	if( inet_aton(sock->host, &inaddr) )
	{
		hints.ai_flags |= AI_NUMERICHOST;
	}

	snprintf(service, sizeof(service), "%d", sock->port);

	if( (rv=getaddrinfo(sock->host, service, &hints, &res)) )
	{
		log_error("getaddrinfo() parser [%s:%s] failed: %s\n", sock->host, service, gai_strerror(rv));
		return -3;
	}

	for (rp=res; rp!=NULL; rp=rp->ai_next)
	{	
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if( sockfd < 0)
		{
			log_error("socket() create failed: %s\n", strerror(errno));
			rv = -3;
			continue;
		}

		/*  connect to server */
		rv = connect(sockfd, rp->ai_addr, len);
		
		if( 0 == rv )
		{
			sock->fd = sockfd;
			log_info("Connect to server[%s:%d] on fd[%d] successfully!\n", sock->host, sock->port, sockfd);
			break;
		}
		else
		{
			/*  socket connect get error, try another IP address */
			close(sockfd);
			continue;
		}
	}

	freeaddrinfo(res);
	return rv;
}


int socket_status( socket_ctx_t *sock )
{
	struct tcp_info 		info;
	socklen_t 				info_len = sizeof(info);
	int               		changed = 0;

	if( !sock )
	{
		return 0;
	}

	if( sock->fd < 0 )
	{
		/*  socket is connected before but got disconnected now */
		changed = sock->connected ? 1 : 0;
		sock->connected = 0;
		goto out;
	}

	getsockopt(sock->fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
	
	if( TCP_ESTABLISHED==info.tcpi_state )
	{
		/*  socket is disconnected before but got connected now */
		changed = !sock->connected ? 1 : 0;
		sock->connected = 1;
	}

	else
	{
		/*  socket is connected before but got disconnected now */
		changed = sock->connected ? 1 : 0;
		sock->connected = 0;
	}

out:
	if( changed )
	{
		log_info("socket status got %s\n", sock->connected?"connected":"disconnected");
	}

	return sock->connected;
}


int socket_send(socket_ctx_t *sock, char *data, int length)
{
	int total_sent = 0;         //累计发送量
	int rv = 0 ; 

	while (total_sent < length) 
	{    
		rv = write(sock->fd, data + total_sent, length - total_sent);
 
		if (rv < 0)
		{
			log_error("socket[%d] write() failure: %s, close socket now\n", sock->fd, strerror(errno));
			return -2;
		}

		if (rv == 0)
		{

			log_error("socket[%d] write() returned 0, connection might be closed by peer\n", sock->fd);
			return -2;
		}

		total_sent += rv; 
	}  

	return 0;
}











