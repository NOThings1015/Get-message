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


int socket_client_init(const char *server_ip, int server_port)
{
	struct sockaddr_in      serv_addr;
	int                     connfd;

	connfd=socket(AF_INET, SOCK_STREAM, 0); 
	if(connfd < 0)
	{   
		printf("Create socket failure: %s",strerror(errno));
		return -1; 
	}   

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(server_port);

	if( inet_aton(server_ip, &serv_addr.sin_addr) == 0)  
	{   
		printf("Invalid IP address: %s\n", server_ip);
		close(connfd);
		return -2;                               
	}   

	if( connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{   
		close(connfd);  
		return -3; 
	}   

	return connfd;

}



void Print_Client_Usage(char *progname)
{
	printf("%s usage method:\n",progname);
	printf("-i(--ip): Specify server IP address to connect.\n");
	printf("-p(--port): Specify server port to connect.\n");
	printf("-t(--time): Client message sending interva.\n"); 
	printf("-h(--help): Printf this help information.\n");
	return ;
}



void Print_Server_Usage(char *progname)
{
	printf("%s usage method:\n",progname);
	printf("-p(--port):sepcify serveer listen port.\n");    
	printf("-h(--help):printf this help information.\n");
	return ;
}





