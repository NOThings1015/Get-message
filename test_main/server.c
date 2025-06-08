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

#include "sqlite.h"
#include "socket.h"
#include "logger.h" 
#include "packet.h"

#define	 Max_event	512

void Print_Server_Usage(char *progname);

int main(int argc, char **argv)
{

	pack_info_t 			*pack_info;


	char					*progname=NULL;
	int						rv=-5;
	int						connfd=-1;

	int 					i=0;
	char					buf[200];
	char					ack[128];
	int            		    serv_port=0;
	int           		    ch=0;
	int						listenfd;

	int						epollfd;			//epoll对象
	struct epoll_event		event;				//events,data
	struct epoll_event		event_array[Max_event];
	int						events;

	char                    *log_file = "server.log";
	int                     log_level = LOG_LEVEL_DEBUG;
	int                     log_size = 1024;

	char                    *sqlite_path="/home/iot25/yangjiayu/Get-message/sqlite3/Storage_temp.db";
	sqlite3					*db;
	struct option   opts[]={
		{"port", required_argument, NULL, 'p'},     // -p 或 --port，需参数（端口号）
		{"help", no_argument, NULL, 'h'},           // -h 或 --help，无需参数
		{NULL, 0, NULL, 0}                          // 结束标记
	};  

	progname = basename(argv[0]);

	// 初始化日志系统
	if (log_open(log_file, log_level, 1024, LOG_LOCK_DISABLE) != 0)
	{
		fprintf(stderr, "Failed to initialize logger.\n");
		return -1;
	}

	log_info("Logger initialized successfully.");


	while((ch = getopt_long(argc, argv, "p:h", opts, NULL)) != -1) 
	{
		switch(ch)
		{   
			case 'p':
				serv_port=atoi(optarg);      //若选项为 -p/--port，将参数值转换为整数并赋值给 port
				log_info("Server port set to: %d", serv_port);
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
		log_error("Invalid arguments. Please specify a port.");
		Print_Server_Usage(argv[0]); 
		return -1; 
	}   

	if( (listenfd=socket_server_init(NULL, serv_port)) < 0)   //创建服务器文件描述符socket
	{
		log_error("Failed to start server on port %d: %s", serv_port, strerror(errno));
		return -2;
	}

	log_info("Server started and listening on port %d", serv_port);


	if( (epollfd=epoll_create(Max_event)) < 0) //创建epoll对象
	{
		log_error("epoll_create() failure: %s", strerror(errno));
		return -3;
	}

	event.events = EPOLLIN;
	event.data.fd = listenfd;
	if( epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0) //向epoll对象中添加套接字listenfd
	{
		log_error("Failed to add listen socket to epoll: %s", strerror(errno));
		return -4;
	}


	if( sqlite_init(sqlite_path, &db) < 0  )
	{
		log_error("Failed to open SQLite database: %s", sqlite_path);
		return -1;
	}

	log_info("SQLite database initialized successfully.");

	for( ; ; )
	{
		events = epoll_wait(epollfd, event_array, Max_event, -1);  //阻塞等待事件发生
		if(events < 0)
		{
			log_error("epoll_wait failure: %s", strerror(errno));
			break;
		}

		if(events == 0)
		{
			log_info("epoll_wait timeout.");
			continue;
		}

		for(i=0; i<events; i++)
		{
			if( (event_array[i].events&EPOLLERR) || (event_array[i].events&EPOLLHUP) )
			{
				log_error("epoll_wait get error on fd[%d]: %s", event_array[i].data.fd, strerror(errno));
				epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
				close(event_array[i].data.fd);
			}

			if(event_array[i].data.fd == listenfd)   //listenfd发生事件，即有新客户端连接
			{
				if( (connfd=accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0)
				{
					log_error("Failed to accept new client: %s", strerror(errno));
					continue;
				}

				event.data.fd = connfd;
				event.events = EPOLLIN|EPOLLOUT;

				if( epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event) < 0)
				{
					log_error("Failed to add client socket to epoll: %s", strerror(errno));
					close(event_array[i].data.fd);
					continue;
				}
				printf("New client connected: fd = %d\n", connfd);
			}

			else  //已连接的客户端有数据收发
			{

				memset(buf, 0, sizeof(buf));

				if( (rv=read(event_array[i].data.fd, buf, sizeof(buf))) <= 0 )
				{
					log_info("Client disconnected or read failure: fd = %d", event_array[i].data.fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
					close(event_array[i].data.fd);

				}	


				packet_tlv_unpack((uint8_t *)buf, rv, pack_info);

/*  
				char     strtime[1024]="";
				char     buflllll[1024]="";
				strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", &pack_info->sample_time);
				snprintf(buflllll, sizeof(buflllll), "{\"devid\":\"%s\", \"time\":\"%s\",\"temperature\":\"%.3f\"}", pack_info->devid, strtime, pack_info->temper);
				printf("sample_data:%s\n",buflllll);
*/

				if( rv > 0 )
				{
					log_info("Received %d bytes from client fd = %d: %s", rv, event_array[i].data.fd, buf);
					if( (sqlite_write(db, buf, rv)) < 0 )     //上传数据存储到指定库文件里面
					{
						log_error("Failed to write to SQLite database.");
						return -2;
					}

				}


			}

		}
	}
CleanUp:
	close(listenfd);
	sqlite3_close(db);
	log_info("Server shutting down.");
	log_close();
	return 0;
}



void Print_Server_Usage(char *progname)
{
	log_info("%s usage method:", progname);
	log_info("-p(--port): Specify server port to listen on.");
	log_info("-h(--help): Print this help information.");
}

