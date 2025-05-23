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
#include <ctype.h> 

#include "loca_time.h"
#include "sqlite.h"
#include "socket.h"


#define	 Max_event	512


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

	
	char                    sqlite_path[128]="/home/iot25/yangjiayu/Get-message/src/../sqlite3/Storage_temp.db";
	sqlite3					*db;
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
					Print_Server_Usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
					return -1; 
			default:
					Print_Server_Usage(argv[0]); 
					break;
		}   
	}   

	if(!serv_port)
	{   
		Print_Server_Usage(argv[0]); 
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


	if(( db = sqlite_open(sqlite_path)) == NULL )
	{
			fprintf(stderr, "Failed to open sqlite.\n");
			return -1;

	}


	if (create_table(db) != 0) //创建temperature表
	{
			fprintf(stderr, "Failed to create table.\n");
			return -1;
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

				memset(buf, 0, sizeof(buf));

				if( (rv=read(event_array[i].data.fd, buf, sizeof(buf))) <= 0 )
				{
					printf("socket[%d] read failure or get disconnect and will be removed.\n", event_array[i].data.fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
					close(event_array[i].data.fd);
					
				}	
					
				
				printf("socket[%d] read %d bytes data: %s\n", event_array[i].data.fd, rv, buf);

				if( (sqlite_write(db, buf)) < 0 )     //上传数据存储到指定库文件里面
				{
						printf("Write to sqlite failure.\n");
						return -2;
				}

				if( (write(event_array[i].data.fd, buf, rv)) < 0 )
				{
					printf("socket[%d] write failure: %s\n", event_array[i].data.fd, strerror(errno));
					epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
					close(event_array[i].data.fd);
				}

				//printf("Write to connfd : %s\n", buf);
			}

			
		}
	}
CleanUp:
	close(listenfd);
	return 0;
}
				





