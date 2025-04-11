/*********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  ds18b20.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(05/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "05/04/25 14:57:33"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h> 
#include <fcntl.h>   
#include <unistd.h>  
#include <stdlib.h>

#include "loca_time.h"

#define buf_size 200

int  get_ds18b20(char *serial_number, int buffer_size, float *temp);
char *generate_sensor_message(void);


char *generate_sensor_message(void)
{

		int			mes_size = 128;
		char 		time[52];             // 时间缓冲区
		char 		serial_number[32];    // 设备号缓冲区
		float 		temp;                // 温度值
		char 		*message = NULL;      // 返回的报文

		// 获取时间
		if (get_time(time, sizeof(time)) < 0)
		{
				fprintf(stderr, "Get time failure.\n");
				return NULL;
		}

		// 获取传感器数据
		if (get_ds18b20(serial_number, sizeof(serial_number), &temp) < 0)
		{
			 	fprintf(stderr, "ds18b20 get temperature failure: %s\n", strerror(errno));
				return NULL;
		}

		// 分配内存
		if ((message = malloc(mes_size)) == NULL)
		{
			  	fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
				return NULL;
		}

		// 格式化报文
		int len = snprintf(message, mes_size, "%s: %s, temperature: %.3f\n", time, serial_number, temp);

		// 检查格式化结果
		if (len < 0 || len >= mes_size)
		{
			  	fprintf(stderr, "Message formatting failed\n");
				free(message);
				return NULL;
		}

		return message;
}




int  get_ds18b20(char *serial_number, int buffer_size, float *temp)
{
	int					fd = -1;
	char				buf[buf_size];
	char				*ptr;

	char    			w1_path[64] = "/sys/bus/w1/devices/";
	DIR					*dirp = NULL;
	struct dirent		*direntp = NULL;		

	int					found = 0;
	char				device_path[200] = ""; //存放数据所在文件夹
//	char				chip_path[32]="";   //存放产品序列号

	if( (dirp = opendir(w1_path)) == NULL )  //按路径打开文件夹，获取DIR句柄
	{
		printf("open folder %s failure: %s\n", w1_path, strerror(errno));
		return -1;
	}

	while( !found )
	{
		if( (direntp=readdir(dirp)) != NULL )  //遍历路径下所有文件
		{
			//printf("failname: %s\n", direntp->d_name);  //可用于打印目录文件
		
			if( strstr(direntp->d_name, "28-") )  //找到ds18b20数据所在文件
			{
				found = 1;	
			}
		}

		else
			break;
	}

	if( !found )
	{
		printf("Can not found ds18b20 message.\n");
		return -2;
	}
		
	
	strncpy(serial_number, direntp->d_name, buffer_size - 1);
	serial_number[buffer_size - 1] = '\0';  // 强制终止

    snprintf(device_path, sizeof(device_path), "%s%s/w1_slave", w1_path, direntp->d_name); //将文件拼接完整
     
	//printf("device_path:%s\n",device_path);		//ds18b20完整路径
	 

	if( (fd = open(device_path, O_RDONLY)) < 0 ) //打开文件 
	{
		printf("open fail %s failure: %s\n", device_path, strerror(errno));
		return -3;
    }   
	
	memset(buf, 0, sizeof(buf));

	if( read(fd, buf, sizeof(buf)) < 0 )  //读取文件数据
	{
		printf("read data from fd=%d failure: %s\n", fd, strerror(errno));
	 	return -4;
	}

	//printf("Get buf: %s\n", buf);

	ptr = strstr(buf, "t="); 		//找到温度数据位置
	if( !ptr )
	{
		printf("Can not find t= string\n");
		return -5;
	}

	ptr += 2;

	*temp = atof(ptr)/1000;
	//printf("Get temprature:  %.3f\n", temp); //打印温度

	closedir(dirp);
	return 0;
}









