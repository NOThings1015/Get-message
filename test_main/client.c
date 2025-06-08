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
#include <sqlite3.h>

#include "ds18b20.h"
#include "sqlite.h"
#include "socket.h"
#include "packet.h"
#include "logger.h"


void print_client_usage(char *progname)
{
	log_info("%s usage method:", progname);
	log_info("-i(--ip): Specify server IP address to connect.");
	log_info("-p(--port): Specify server port to connect.");
	log_info("-t(--time_interval): Client message sending interval.");
	log_info("-d(--debug): Running to debug.");
	log_info("-h(--help): Print this help information.");
}

void handle_sigpipe(int sig); 
int check_sample_time(time_t *last_time, int interval);


int main(int argc, char **argv)
{
	int						daemon;
	int             		server_port = 0;
	int						ch = 0;
	int             		connfd = -1;
	char					*server_ip = NULL;
	char					buf[1024];
	socket_ctx_t         	sock;

	int						interval_time;			
	time_t 					last_time = 0;
	
	char                    sample_time[52];
	char					w_message[256] = "";
	float					temp;
	char					serial_number[32] = "";


 	pack_info_t          	pack_info;
	pack_proc_t				pack_proc = packet_tlv_pack;

	char					pack_buf[1024];
	int						pack_bytes = 0;

	char 					*log_file = "client.log";
	int 					log_level = LOG_LEVEL_DEBUG;
	int 					log_size = 1024;
	char            		*sqlite_path="/home/iot25/yangjiayu/Get-message/sqlite3/Temp.db";
	int    					conn_flag = 0;
	
	int 					mess_flage = 0;
	int						rv = -1;
	int                     error = 0;
	struct sockaddr_in		serv_addr;
	struct tcp_info 		info;
	socklen_t 				info_len = sizeof(info);
	sqlite3					*db;

	struct option   		opts[]={
		{"ipaddr",   required_argument, NULL, 'i'},    	// -i 或 --ipaddr， 需参数（服务器地址）
		{"port", required_argument, NULL, 'p'},     	// -p 或 --port，需参数（服务器端口号）
		{"time_interval", required_argument, NULL, 't'},// -t
		{"debug", no_argument, NULL, 'd'},				// -d
		{"help", no_argument, NULL, 'h'},           	// -h 或 --help，无需参数
		{NULL, 0, NULL, 0}                          	// 结束标记
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
				interval_time = atoi(optarg);
				break;

			case 'd':
				daemon  = 0;
				log_file = "console";
				log_level = LOG_LEVEL_DEBUG;
				break;

			case 'h':	
				print_client_usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
				return 0;
			
			default:
				print_client_usage(argv[0]);
				return -1;
		}
	}

	if (!server_ip || !server_port || (interval_time <= 0) )
	{
		print_client_usage(argv[0]);
		return -2;
	}

	if( sqlite_init(sqlite_path, &db) < 0 )
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
	
	socket_init(&sock, server_ip, server_port);

	while(1)				  
	{	

		//单位时间采样：
		mess_flage = 0;
		
		if( check_sample_time(&last_time, interval_time) )
		{
			//获取温度
			if (get_ds18b20_tmp(&pack_info.temper) < 0 )
			{
				log_error("ds18b20 get temperature failure: %s\n", strerror(errno));
			}

			//获取时间
			get_time(&pack_info.sample_time);

			//获取产品序列号
			get_serial_number(pack_info.devid, sizeof(pack_info.devid), 3);
			
			//格式化采样数据
			pack_bytes = pack_proc(&pack_info, (uint8_t *)pack_buf, sizeof(pack_buf));

			mess_flage = 1;
			log_info("Get temperature message: %s", pack_buf);
		}
		
		// 在发送数据之前检查连接状态
		conn_flag = socket_status(&sock);  
		
		if(conn_flag == 0 )				
		{
			//断线重连
			if( socket_connect(&sock) < 0 )
			{
				//有数据则暂存
				if( mess_flage == 1 )
				{
					if (sqlite_write(db, pack_buf, pack_bytes) < 0)
					{
						printf("write to sqlite error\n");
					}
					
					log_info("Message stored in database due to connection failure.");
				}	

				continue;
			}
			
			conn_flag = 1;
			log_info("Connected to server: [%s:%d]\n", sock.host, sock.port);

		}

		//上报数据					
		if( mess_flage == 1 )		
		{
			if (socket_send(&sock, pack_buf, pack_bytes) < 0) 
			{
				log_error("Failed to send message to server: [%s:%d]", sock.host, sock.port);
				sqlite_write(db, pack_buf, pack_bytes);	//发送失败，写入数据库
				socket_terminate(&sock);
				continue; //触发重连
			}	

			log_info("Message sent successfully: %s", pack_buf);
			mess_flage = 0;
		}
		
		//发送/清空数据库中数据
		if( !sqlite_check_read(db, pack_buf, sizeof(pack_buf), &pack_bytes) )		
		{
			if (socket_send(&sock, pack_buf, pack_bytes) < 0) 
			{
				log_error("Failed to get message from sqlite: %s", sqlite_path);
				continue;
			}

			mess_flage = 1;

			//发送读取的数据
			if (socket_send(&sock, pack_buf, pack_bytes) < 0) 
			{
				log_error("Failed to send message to server: [%s:%d]", sock.host, sock.port);
				socket_terminate(&sock);
				continue; //触发重连
			}	

			mess_flage = 0;

			log_info("Message sent successfully: %s", pack_buf);

			//删除数据库中已发送的数据
			if ( delete_1st_row(db) < 0 )  	
			{
				log_error("Failed to delete message from sqlite: %s", sqlite_path);
			}

			log_info("First row deleted from database.");
		}
	}
	
	if (connfd != -1) 
	{
		socket_terminate(&sock);
		sqlite3_close(db);	

		log_info("Connection closed and database closed.");
		log_close();
	}

	return 0;
}



void handle_sigpipe(int sig) 
{
	log_info("SIGPIPE signal received, ignoring...");    
	return;

}


int check_sample_time(time_t *last_time, int interval_time)
{
	int                  need = 0; /*  no need sample now */
	time_t               now;

	time(&now);

	if( difftime(now, *last_time) >= interval_time )
	{
		need = 1; /*  need sample now  */
		*last_time = now;
	}

	return need;
}






















