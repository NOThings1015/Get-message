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


#include "ds18b20.h"


void print_usage(char *progname);

int server_client_init(const char *server_ip, int server_port);



int main(int argc, char **argv)
{

	int             		server_port = 0;
	int						ch = 0;
	int             		connfd = -1;
	char					*server_ip = NULL;
	char					buf[200];

	char					w_message[200] = "";
	float					temp;
	char					serial_number[32] = "";

	int						rv = -1;
	struct sockaddr_in		serv_addr;

	struct option   opts[]={
		{"ip",   required_argument, NULL, 'i'},    // -i 或 --ip， 需参数（服务器地址）
		{"port", required_argument, NULL, 'p'},     // -p 或 --port，需参数（服务器端口号）
		{"help", no_argument, NULL, 'h'},           // -h 或 --help，无需参数
		{NULL, 0, NULL, 0}                          // 结束标记
	};

	while((ch = getopt_long(argc, argv, "i:p:h", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
					server_ip = strdup(optarg);  // 复制字符串
					break;
			case 'p':
					server_port = atoi(optarg);      //若选项为 -p/--port，将参数值转换为整数并赋值给 port
					break;
			case 'h':
					print_usage(argv[0]);   //若选项为 -h/--help，调用 print_usage 并返回 0。
					return 0;
			default:
					print_usage(argv[0]);
					return -1;
		}
	}

	if (!server_ip) 
	{
		print_usage(argv[0]);
		return -2;
	}

	if(!server_port)
	{
		print_usage(argv[0]);
		return -3;
	}
					      
				  
	if( (connfd=server_client_init(server_ip, server_port)) < 0)
	{
		printf("connect to server [%s:%d] dailure: %s\n",*server_ip, server_port, strerror(errno));
		return -4;
	}
	

	if( (get_ds18b20(serial_number, sizeof(serial_number), &temp)) < 0 )  //获取设备版本号和温度
	{
		printf("ds18b20 get temprature failure: %s\n", rv);
		
	}

	snprintf(w_message, sizeof(w_message), "Get serial_number: %s, temprature: %.3f", serial_number,  temp);  // 温度格式化为字符串，如 "25.500"

	while(1)
	{
		if( write(connfd, w_message, strlen(w_message)) < 0 )
		{
			printf("Write data to server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
	    	close(connfd);
			return -5;
		}
		
		
		sleep(10);

		memset(buf, 0, sizeof(buf));
		rv = read(connfd, buf, sizeof(buf));
		if(rv < 0)
		{
			printf("Read data from server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
			close(connfd);
			return -6;
		}

		else if( 0 == rv)
		{
			printf("Client connect to server get disconnected\n");
			close(connfd);
			return -7;
		}	
		printf("Read %d bytes data from server: %s\n", rv, buf);
	}

CleanUp:
	close(connfd);
	
}







int server_client_init(const char *server_ip, int server_port)
{
	struct sockaddr_in		serv_addr;
	int						connfd;

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
		printf("Connect to server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
		close(connfd);	
		return -3;
	}

	return connfd;

}


int client_write(int connfd, const char *w_message, const char *server_ip, int server_port)
{
	if( write(connfd, w_message, strlen(w_message)) < 0 )    
	{
		printf("Write data to server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno)); 
		close(connfd);
		return -1;
	} 

	return 0;
}

int client_read(int connfd, char *buf, int buf_size, const char *server_ip, int server_port)
{
	int				rv=-1;

	memset(buf, 0, buf_size);

	rv=read(connfd, buf, buf_size-1);
	if(rv < 0)
	{
		printf("Read data from server [%s:%d] failure: %s\n", server_ip, server_port, strerror(errno));
		close(connfd);
		return -1;
	}

	else if( 0 == rv)
	{
		printf("Client connect to server get disconnected\n");
		close(connfd);
		return -2;
	}

	buf[rv] = '\0';
	return rv;
}



void print_usage(char *progname)
{
	printf("%s usage method:\n",progname);
	printf("-i(--ip):Specify server IP address to connect\n");
	printf("-p(--port): Specify server port to connect \n");
	printf("-h(--help):Printf this help information.\n");
	return ;
}



