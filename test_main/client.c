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

#include <getopt.h>
#include <stdlib.h>
#include <ctype.h> 

#include "ds18b20.h"
#include "loca_time.h"
#include "sqlite.h"
#include "send.h"
#include "socket.h"




int main(int argc, char **argv)
{

		int             		server_port = 0;
		int						ch = 0;
		int             		connfd = -1;
		char					*server_ip = NULL;
		char					buf[1024];

		char					*w_message;
		float					temp;
		char                    time[52];			//采样时的具体时间
		int						TIME_I;				//采样时间间隔(单位秒)

		char					serial_number[32] = "";


		char            		sqlite_path[128]="/home/iot25/yangjiayu/Get-message/src/../sqlite3/Temp.db";
		char					output_file[128]="/home/iot25/yangjiayu/Get-message/src/../tmp/output.txt";

		int 					read_flage = 0;

		int						rv = -1;
		struct sockaddr_in		serv_addr;

		struct option   opts[]={
			{"ip",   required_argument, NULL, 'i'},    // -i 或 --ip， 需参数（服务器地址）
			{"port", required_argument, NULL, 'p'},     // -p 或 --port，需参数（服务器端口号）
			{"time", required_argument, NULL, 't'},		// -t
			{"help", no_argument, NULL, 'h'},           // -h 或 --help，无需参数
			{NULL, 0, NULL, 0}                          // 结束标记
		};

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
								TIME_I = atoi(optarg);
								break;

						case 'h':	
								Print_Client_Usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
								return 0;
						default:
								Print_Client_Usage(argv[0]);
								return -1;
				}
		}

		if (!server_ip) 
		{
				Print_Client_Usage(argv[0]);
				return -2;
		}

		if(!server_port)
		{
				Print_Client_Usage(argv[0]);
				return -3;
		}

		if (TIME_I <= 0) 
		{
				fprintf(stderr, "Error: Time interval (-t) must be a positive integer.\n");
		        Print_Client_Usage(argv[0]);
		        return -4;
	    }
					      
		while(1)				  
		{	
				


				w_message = generate_sensor_message();	 //格式化获取：时间、设备号、温度为字符串

				if( (connfd=server_client_init(server_ip, server_port)) < 0) //若服务器连接失败，数据临时存储到sqlite数据库Temp.db的表TempData下
				{
						printf("connect to server [%s:%d] failure,sending to Sqlite of Temp.db,retrying in 5 seconds...\n", server_ip, server_port);
				
						create_table(sqlite_path);
						sqlite_write(sqlite_path, w_message);
		
						sleep(5); //等待3s后重试
						continue;
				}


				send_not_empty(sqlite_path, output_file, connfd);//重连成功，发送所有暂存数据给服务器,并清空

				while(1)
				{	
						
						if (send_all(connfd, w_message, strlen(w_message)) < 0) //使用 send_all() 完整发送当前数据
						{
								printf("Write data to server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
	    						close(connfd);
								connfd = -1;
								break; // 跳出内层循环，触发重连
						}
	

						sleep(TIME_I);          //每TIME_I秒上报一次数据

						w_message = generate_sensor_message();   //格式化获取：时间、设备号、温度为字符串

						
						memset(buf, 0, sizeof(buf));
						rv = read(connfd, buf, sizeof(buf));
						if(rv < 0)
						{
								printf("Read data from server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
								close(connfd);
								connfd = -1;
								break; // 跳出内层循环，触发重连
						}

						else if( 0 == rv)
						{
								printf("Client connect to server get disconnected\n");
								close(connfd);
								connfd = -1;
								break; // 跳出内层循环，触发重连
						}	
						printf("Read %d bytes data from server: %s\n", rv, buf);
				}

				if (connfd != -1) 
				{
						shutdown(connfd, SHUT_RDWR); // 确保双向关闭
						close(connfd);
						memset(w_message, 0, sizeof(w_message));
						connfd = -1;
				}
			
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



