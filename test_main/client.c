/*********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  client.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(02/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "02/04/25 08:56:00"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h> 
#include <time.h>
#include <netinet/tcp.h>
#include "logger.h"
#include <sqlite3.h>
#include "ds18b20.h"
#include "loca_time.h"
#include "sqlite.h"
#include "send.h"
#include "socket.h"
#include "logger.h"

void handle_sigpipe(int sig) 
{
	log_info("SIGPIPE signal received, ignoring...");    
	return;

}


int main(int argc, char **argv)
{

	int             		server_port = 0;
	int						ch = 0;
	int             		connfd = -1;
	char					*server_ip = NULL;
	char					buf[1024];
	char					w_message[256] = "";
	float					temp;
	int						time_interval;				//采样时间间隔(单位秒)
	char					sample_time[52];
	time_t 					last_time = 0;
	time_t					current_time = 0;
	char					serial_number[32] = "";
	char 					*log_file = "client.log";
	int 					log_level = LOG_LEVEL_DEBUG;
	int 					log_size = 1024;
	char					serimal_number[32] = "";
	char            		sqlite_path[128]="../sqlite3/Temp.db";
	int    					conn_flag = 0;
	char*					table_name = "TempData";
	int 					mess_flage = 0;
	int						rv = -1;
	int                     error = 0;
	struct sockaddr_in		serv_addr;
	struct tcp_info 		info;
	socklen_t 				info_len = sizeof(info);
	sqlite3					*db;

	struct option   		opts[]={
		{"ip",   required_argument, NULL, 'i'},    // -i 或 --ip， 需参数（服务器地址）
		{"port", required_argument, NULL, 'p'},     // -p 或 --port，需参数（服务器端口号）
		{"time", required_argument, NULL, 't'},		// -t
		{"help", no_argument, NULL, 'h'},           // -h 或 --help，无需参数
		{NULL, 0, NULL, 0}                          // 结束标记
	};

	//命令行参数解析
	while((ch = getopt_long(argc, argv, "i:p:t:h", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
				server_ip = strdup(optarg);  // 复制字符串
				break;
			case 'p':
				server_port = atoi(optarg);      //若选项为 -p/--port，将参数值转换为整数并赋值给 port
				break;
			case 't':
				time_interval = atoi(optarg);
				break;

			case 'h':	
				Print_Client_Usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
				return 0;
			default:
				Print_Client_Usage(argv[0]);
				return -1;
		}
	}

	if (!server_ip || !server_port || (time_interval <= 0) )
	{
		Print_Client_Usage(argv[0]);
		return -2;
	}

	if(( db = sqlite_open(sqlite_path)) == NULL )
	{
		printf("Failed to open SQLite database: %s", sqlite_path);
		return -1;
	}   

	signal(SIGPIPE, handle_sigpipe); // 捕获 SIGPIPE 信号					


	// 初始化日志系统
	if (log_open(log_file, log_level, 1024, LOG_LOCK_DISABLE) != 0)
	{
		fprintf(stderr, "Failed to initialize logger.\n");
		return -1;
	}

	while(1)				  
	{	

		//单位时间采样：
		mess_flage = 0;
		time(&current_time);    
		if( (difftime(current_time, last_time) >= time_interval ))
		{
			memset(w_message, 0, sizeof(w_message));
			
			//获取时间
			if (get_time(sample_time, sizeof(sample_time)) < 0)
			{
				log_error("Get time failure.\n");
			}

			//获取产品序列号
			if ((rv = get_custom_serial(serial_number, sizeof(serial_number))) < 0 )
			{
				log_error("Get custom serial number failured\n");
			}

			//获取温度
			if (get_ds18b20_tmp(serial_number, &temp) < 0 )
			{
				log_error("ds18b20 get temperature failure: %s\n", strerror(errno));
			}

			//格式化采样数据
			rv = snprintf(w_message, sizeof(w_message), "%s: %s, temperature: %.3f\n", sample_time, serial_number, temp);

			if (rv < 0 || rv >= sizeof(w_message))
			{
				log_error("Message formatting failed\n");
			}

			last_time = current_time;
			mess_flage = 1;
			log_info("Get temperature message: %s", w_message);
		}


		// 在发送数据之前检查连接状态
		conn_flag = socket_connected(&connfd) ? 0 : 1; 

		//断线重连
		if(conn_flag == 0 )				
		{
			if(( connfd = socket_client_init(server_ip, server_port, &serv_addr)) < 0 )
			{
				log_error("Failed to initialize client socket.");
				continue;
			}


			if(connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
			{
				log_error("Failed to connect to server: [%s:%d]", server_ip, server_port); 
				close(connfd);
				connfd = -1;
				if( mess_flage == 1 )
				{
					sqlite_write(db, w_message);
					log_info("Message stored in database due to connection failure.");
				}	
				continue;
			}

			log_info("Connected to server: [%s:%d]\n", server_ip, server_port);
			conn_flag = 1;
		}

		//上报数据					
		if( mess_flage == 1 )		//如果有数据则发送
		{
			if (send_message(connfd, w_message, strlen(w_message)) < 0) //发送当前数据
			{
				log_error("Failed to send message to server: [%s:%d]", server_ip, server_port);

				sqlite_write(db, w_message);	//发送失败，写入数据库
				
				close(connfd);
				connfd = -1;
				conn_flag = 0;
				continue; // 跳出内层循环，触发重连
			}	

			log_info("Message sent successfully: %s", w_message);
			mess_flage = 0;
		}


		//发送清空数据库中数据
		if(!is_database_empty(db))		
		{
			//读取数据库第一行
			if ( (sqlite_read_1st(db, w_message, sizeof(w_message))) <  0 )
			{
				log_error("Failed to get message from sqlite: %s", sqlite_path);
				continue;
			}

			mess_flage = 1;
			//发送读取的数据
			if (send_message(connfd, w_message, strlen(w_message)) < 0) //发送当前数据
			{
				log_error("Failed to send message to server: [%s:%d]", server_ip, server_port);

				close(connfd);
				connfd = -1;
				conn_flag = 0;
				continue; // 跳出内层循环，触发重连
			}	

			mess_flage = 0;

			log_info("Message sent successfully: %s", w_message);

			//删除数据库中已发送的数据
			if ( delete_1st_row(db, table_name) < 0 )  	//成功读取数据则删除缓存区
			{
				log_error("Failed to delete message from sqlite: %s", sqlite_path);
			}

			log_info("First row deleted from database.");
		}
	}

	if (connfd != -1) 
	{
		sqlite3_close(db);	//关闭数据库
		close(connfd);		//关闭套接字
		connfd = -1;
		log_info("Connection closed and database closed.");
	}

	return 0;
}


void Print_Client_Usage(char *progname)
{
	log_info("%s usage method:", progname);
	log_info("-i(--ip): Specify server IP address to connect.");
	log_info("-p(--port): Specify server port to connect.");
	log_info("-t(--time): Client message sending interval.");
	log_info("-h(--help): Print this help information.");
}

