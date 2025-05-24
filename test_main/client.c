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
#include <sqlite3.h>
#include "ds18b20.h"
#include "loca_time.h"
#include "sqlite.h"
#include "send.h"
#include "socket.h"
#include <netinet/tcp.h>


void handle_sigpipe(int sig) 
{
	    // 忽略 SIGPIPE 信号，不进行任何处理
	     return;

}


int main(int argc, char **argv)
{

		int             		server_port = 0;
		int						ch = 0;
		int             		connfd = -1;
		char					*server_ip = NULL;
		char					buf[1024];
		char					ack[256] = "";
		char					w_message[256] = "";
		float					temp;
		int						time_set;				//采样时间间隔(单位秒)

		time_t 					start = 0;
		time_t					end = 0;
		char					serial_number[32] = "";

		char            		sqlite_path[128]="../sqlite3/Temp.db";
		char					output_file[128]="../tmp/output.txt";
		sqlite3					*db;

		int 					mess_flage = 0;

		int						rv = -1;
		struct sockaddr_in		serv_addr;
		int						error = 0;
		socklen_t 				len = sizeof(error);


		int    					conn_flag = 0;
		char*					table_name = "TempData";

		struct option   opts[]={
			{"ip",   required_argument, NULL, 'i'},    // -i 或 --ip， 需参数（服务器地址）
			{"port", required_argument, NULL, 'p'},     // -p 或 --port，需参数（服务器端口号）
			{"time", required_argument, NULL, 't'},		// -t
			{"help", no_argument, NULL, 'h'},           // -h 或 --help，无需参数
			{NULL, 0, NULL, 0}                          // 结束标记
		};
		
		struct tcp_info
		{
			    unsigned char tcpi_state;
		};

		struct tcp_info 		info;
		socklen_t 				info_len = sizeof(info);

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
								time_set = atoi(optarg);
								break;

						case 'h':	
								Print_Client_Usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
								return 0;
						default:
								Print_Client_Usage(argv[0]);
								return -1;
				}
		}

		if (!server_ip || !server_port || (time_set <= 0) )
		{
				Print_Client_Usage(argv[0]);
				return -2;
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

		signal(SIGPIPE, handle_sigpipe); // 捕获 SIGPIPE 信号					

		while(1)				  
		{	

				//单位时间采样：
				mess_flage = 0;
				time(&end);    
				if( (difftime(end, start) >= time_set ))
				{
						memset(w_message, 0, sizeof(w_message));
						if (generate_sensor_message(w_message, sizeof(w_message)) == 0)	
						{
								start = end;
								mess_flage = 1;
						}

						else
						{
								printf("Get w_message failure.\n");
						}
				}


				//断线重连
				if(conn_flag == 0 )				
				{
						if(( connfd = socket_client_init(server_ip, server_port, &serv_addr)) < 0 )
						{
								printf("Client socket init failure\n");
						}
						
				
						if(connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
						{
								//printf("Client connect to server: [%s:%d] failure\n", server_ip, server_port); 
								close(connfd);
						}
						
						if((getsockopt(connfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) || (error != 0) )
						{
								close(connfd);
								connfd = -1;
								if( mess_flage == 1 )
								{
										sqlite_write(db, w_message);
								}	
								continue;
						}

						conn_flag = 1;
				}
						

				// 在发送数据之前检查连接状态
				if( (getsockopt(connfd, IPPROTO_TCP, TCP_INFO, &info, &info_len) < 0) || (info.tcpi_state != TCP_ESTABLISHED))
				{
						printf("getsockopt(TCP_INFO) failed");
						close(connfd);
						connfd = -1;
						conn_flag = 0;
						if (mess_flage == 1)
						{
								sqlite_write(db, w_message);
						}
						continue;
				}


				//上报数据					
				if( mess_flage == 1 )		//如果有数据则发送
				{
						if (send_message(connfd, w_message, strlen(w_message)) < 0) //发送当前数据
						{
								printf("Write data to server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
	    						
								sqlite_write(db, w_message);	//发送失败，写入数据库
								
								close(connfd);
								connfd = -1;
								conn_flag = 0;
								continue; // 跳出内层循环，触发重连
						}	

						printf("Send w_message: %s\n",w_message);
				}

				if(!is_database_empty(db))		//如果数据库非空
				{
						if( (sqlite_read_1st(db, connfd)) == 0 )
						{
								delete_1st_row(db, table_name);	//成功读取数据则删除缓存区
						}
				}
		}

		if (connfd != -1) 
		{
			    sqlite3_close(db);	//关闭数据库
				close(connfd);		//关闭套接字
				connfd = -1;
		}
}


void Print_Client_Usage(char *progname)
{
		printf("%s usage method:\n",progname);
		printf("-i(--ip): Specify server IP address to connect.\n");
		printf("-p(--port):	Specify server port to connect.\n");
		printf("-t(--time):	Client message sending interva.\n"); 
		printf("-h(--help):	Printf this help information.\n");
		return ;
}



