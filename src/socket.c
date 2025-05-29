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


int socket_client_init(const char *server_ip, int server_port, struct sockaddr_in *serv_addr)
{
	int       	            	connfd;

	char 						port_str[16];
	struct addrinfo				hints;
	struct addrinfo				*res;	

	struct addrinfo 			*p;

	//创建套接字
	connfd=socket(AF_INET, SOCK_STREAM, 0); 
	if(connfd < 0)
	{   
		printf("Create socket failure: %s",strerror(errno));
		return -1; 
	}   

	// 检查输入是否为有效的点分十进制 IP 地址
	if (inet_pton(AF_INET, server_ip, &serv_addr->sin_addr) > 0)
	{

		memset(serv_addr, 0, sizeof(struct sockaddr_in));
		serv_addr->sin_family = AF_INET;
		serv_addr->sin_port = htons(server_port);
	}

	else
	{
		//初始化hints结构体
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		snprintf(port_str, sizeof(port_str), "%d", server_port);

		// 使用 getaddrinfo 域名解析
		if (getaddrinfo(server_ip, port_str, &hints, &res) != 0)
		{
			printf("Invalid IP address or domain name: %s\n", server_ip);
			close(connfd);
			return -2;
		}

		for ( p = res; p!= NULL; p = p->ai_next )
		{
			if ( p->ai_family == AF_INET )
			{

				memcpy(serv_addr, res->ai_addr, sizeof(struct sockaddr_in));
				serv_addr->sin_port = htons(server_port);
				break;
			}
		}

		freeaddrinfo(res);

		if ( p == NULL )
		{
			printf("No valid IPv4 address found for %s\n", server_ip);
			close(connfd);
			return -3;
		}
	}
	return connfd;
}





int socket_connected( int *connfd )
{
	struct tcp_info 		info;
	socklen_t 				info_len = sizeof(info);

	// 在发送数据之前检查连接状态
	if( getsockopt(*connfd, IPPROTO_TCP, TCP_INFO, &info, &info_len) < 0 )
	{
		close(*connfd);
		*connfd = -1;
		return 1;
	}

	if( info.tcpi_state != TCP_ESTABLISHED )
	{
		close(*connfd);
		*connfd = -1;
		return 1;
	}

	return 0;

}













