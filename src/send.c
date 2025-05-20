/*********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  send.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "11/04/25 11:57:53"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>  // 用于 fileno 和 ftruncate
#include <time.h>   // 用于 sleep

#include "sqlite.h"



int send_data_from_file(char *output_file, int connfd);

int send_message(int sockfd, const char *data, int length);




int send_not_empty(char *sqlite_path, char *output_file, int connfd)
{
		int      i = 0;
		if(!is_database_empty(sqlite_path)) 
		{
	    		for (int retry = 0; retry < 3; retry++) 
				{
						i = 0;
			        	if (sqlite_read(sqlite_path, output_file) == 0 && send_data_from_file(output_file, connfd) == 0) 
						{
								sqlite_clear(sqlite_path);  // 仅在成功时清除
								i--;		
								break;
						}
				}
		}
		return i;		
}



int send_data_from_file(char *output_file, int connfd)
{
	    //以读写模式打开文件
		FILE *fp = fopen(output_file, "r+");
		if (!fp) 
		{
				fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
				return -1;
		}
	
		char buffer[57];
		long file_pos = 0;
		int send_count = 0;
		
		while (1) 
		{
				// 记录当前位置
				long current_pos = ftell(fp);
				// 精确读取57字节
				size_t read_bytes = fread(buffer, 1, sizeof(buffer), fp);
				if (read_bytes == 0) break;

				// 发送数据块
				if (send_message(connfd, buffer, read_bytes) < 0) 
				{
						fseek(fp, file_pos, SEEK_SET);  // 回退到上次成功位置
						break;
				}

				file_pos = current_pos + read_bytes;  // 更新安全位置
				send_count++;
		}

		if (send_count > 0) 
		{
				ftruncate(fileno(fp), file_pos);
		}
			
		fclose(fp);
		return (send_count > 0) ? 0 : -1;
}



int send_message(int sockfd, const char *data, int length)
{
		int total_sent = 0;			//累计发送量
		

		int rv = 0 ;

		while (total_sent < length) 

		{				
				rv = write(sockfd, data + total_sent, length - total_sent);
							        
				if (rv <= 0) 
				{
						return -1;
				}
				
				total_sent += rv;

		}

		return 0;
	
}





