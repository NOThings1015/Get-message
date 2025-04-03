/*********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  server.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(01/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "01/04/25 13:12:21"
 *                 
 ********************************************************************************/

#include<stdio.h>
//#include<server.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "usage.h"


int socket_server_init(char *listen_ip,int listen_port);

int main(int argc, char **argv)
{
	int 		 lis_port =8888;
	int			 rv=-5;
	int			 connfd=-1;
	usage_message(argc, argv);
	rv=socket_server_init(NULL,lis_port);
	printf("rv=%d\n",rv);
	
	return 0;
}

int socket_server_init(char *listen_ip,int listen_port)
{
	struct sockaddr_in			servaddr;
	int 						rv=0;
	int							on=-1;
	int							listenfd;

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
			goto CleanUp;
		}
	}

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		printf("Use bind() to  bind the TCP socket failure:%s\n",strerror(errno));
		rv = -3;
		goto CleanUp;
	}

	if(listen(listenfd, 200) < 0)
	{
		printf("Use listen() to listen listenfd failure:%s",strerror(errno));
		rv = -4;
		goto CleanUp;
	}

CleanUp:
	if(rv < 0)
		close(listenfd);
	else
		rv = listenfd;

	return rv;

}

