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
#define	 Max_event	512

int socket_server_init(char *listen_ip,int listen_port);
void print_usage(char *progname);


int main(int argc, char **argv)
{
	char					*progname=NULL;
	int						rv=-5;
	int						connfd=-1;

	int 					i=0;
	char					buf[200];
	
	int            		    serv_port=0;
	int           		    ch=0;
	int						listenfd;

	int						epollfd;			//epoll对象
	struct epoll_event		event;				//events,data
	struct epoll_event		event_array[Max_event];
	int						events;

	struct option   opts[]={
		{"port", required_argument, NULL, 'p'},     // -p 或 --port，需参数（端口号）
	    {"help", no_argument, NULL, 'h'},           // -h 或 --help，无需参数
        {NULL, 0, NULL, 0}                          // 结束标记
    };  
	
	progname = basename(argv[0]);

	while((ch = getopt_long(argc, argv, "p:h", opts, NULL)) != -1) 
	{
		switch(ch)
		{   
			case 'p':
					serv_port=atoi(optarg);      //若选项为 -p/--port，将参数值转换为整数并赋值给 port
					break;
			case 'h':
					print_usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
					return -1; 
			default:
					break;
		}   
	}   

	if(!serv_port)
	{   
		print_usage(argv[0]);
		return -1; 
	}   

	if( (listenfd=socket_server_init(NULL, serv_port)) < 0)   //创建服务器文件描述符socket
	{
		printf("Error: %s server listen on port %d failure\n", argv[0], serv_port);
		return -2;
	}

	printf(" %s server start to listen on port %d \n", argv[0], serv_port);
	
	if( (epollfd=epoll_create(Max_event)) < 0) //创建epoll对象
	{
		printf("epoll_create() failure: %s\n", strerror(errno));
		return -3;
	}

	event.events = EPOLLIN;
	event.data.fd = listenfd;
	if( epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0) //向epoll对象中添加套接字listenfd
	{
		printf("epoll add listen socket failure: %s\n", strerror(errno));
		return -4;
	}

	for( ; ; )
	{
		events = epoll_wait(epollfd, event_array, Max_event, -1);  //阻塞等待事件发生
		if(events < 0)
		{
			printf("epoll failure: %s\n",strerror(errno));
			break;
		}

		if(events == 0)
		{
			printf("epoll get timeout\n");
			continue;
		}

		for(i=0; i<events; i++)
		{
			if( (event_array[i].events&EPOLLERR) || (event_array[i].events&EPOLLHUP) )
			{
				printf("epoll_wait get error on fd[%d]: %s\n", event_array[i].data.fd, strerror(errno));
				epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
				close(event_array[i].data.fd);
			}

			if(event_array[i].data.fd == listenfd)   //listenfd发生事件，即有新客户端连接
			{
				if( (connfd=accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0)
				{
					printf("accept new client failure: %s\n", strerror(errno));
					continue;
				}

				event.data.fd = connfd;
				event.events = EPOLLIN|EPOLLOUT;
				
				if( epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event) < 0)
				{
					printf("epoll add client socket failure: %s\n", strerror(errno));
					close(event_array[i].data.fd);
					continue;
				}
				printf("epoll add new client socket[%d] ok.\n", connfd);

			}
			
			
			else  //已连接的客户端有数据收发
			{
				if( (rv=read(event_array[i].data.fd, buf, sizeof(buf))) <= 0)
				{
					printf("socket[%d] read failure or get disconnect and will be removed.\n", event_array[i].data.fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
					close(event_array[i].data.fd);
					continue;
				}

				printf("socket[%d] read get %d bytes data\n", event_array[i].data.fd, rv);

				if( write(event_array[i].data.fd, buf, rv) < 0)
				{
					printf("socket[%d] write failure: %s\n", event_array[i].data.fd, strerror(errno));
					epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
					close(event_array[i].data.fd);
				}
			}

			
		}
	}
CleanUp:
	close(listenfd);
	return 0;
}
				

int socket_server_init(char *listen_ip,int listen_port);


int socket_server_init(char *listen_ip,int listen_port)
{
	struct sockaddr_in			servaddr;
	int 						rv=0;
	int							on=1;
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


void print_usage(char *progname)
{
	printf("%s usage method:\n",progname);
	printf("-p(--port):sepcify serveer listen port.\n");		 
	printf("-h(--help):printf this help information.\n");
	return ;
}



