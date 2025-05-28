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
#include "ds18b20.h"

#define buf_size 200


int generate_sensor_message(char *serial_number, char *buffer, int buffer_size)
{
		int 		mes_size = buffer_size; // 使用传入的缓冲区大小
		char 		time[52];              // 时间缓冲区
		float 		temp;                 // 温度值
		int 		len;
		
		int 		rv = 0;
		if (get_time(time, sizeof(time)) < 0)
		{
				fprintf(stderr, "Get time failure.\n");
				return -1; // 返回错误码
		}

		if ( (rv =get_custom_serial(serial_number, buffer_size)) < 0 )
		{
				printf("Get custom serial number failured\n");
				printf("LLLLLLLL:%d\n",rv);
				return -1;
		}
	
		if (get_ds18b20_tmp(serial_number, &temp) < 0 )
		{
				fprintf(stderr, "ds18b20 get temperature failure: %s\n", strerror(errno));
				return -1; // 返回错误码
		}

		len = snprintf(buffer, mes_size, "%s: %s, temperature: %.3f\n", time, serial_number, temp);


		if (len < 0 || len >= mes_size)
		{
				fprintf(stderr, "Message formatting failed\n");
				return -1; // 返回错误码

		}
		return 0;
}



int  get_ds18b20_tmp(char *serial_number, float *temp)
{
	FILE 			*fp = NULL;
	char 			line[256] = {0};
	char 			saved_serial[32] = {0};
	char 			physical_serial[64] = {0};
	char 			path[256] = {0};
	int 			fd = -1;
	char 			buf[1024] = {0};
	ssize_t 		len = 0;
	char 			*ptr = NULL;
	char	 		*config_file = "/home/iot25/yangjiayu/Get-message/etc/ds18b20.conf";
	

	//参数检查
	if (serial_number == NULL || temp == NULL) 
	{
			return -1;
	}

	if ((fp = fopen(config_file, "r")) == NULL )
	{
		return -2;
	}

	while (fgets(line, sizeof(line), fp) != NULL) 
	{
		if (sscanf(line, "%s %s", saved_serial, physical_serial) == 2) 
		{
			if (strcmp(saved_serial, serial_number) == 0)
			{
				fclose(fp);
		 		snprintf(path, sizeof(path), "/sys/bus/w1/devices/%s/w1_slave", physical_serial);
		 		if( (fd = open(path, O_RDONLY)) < 0 )
				{
						 return -3;
				}
		 		memset(buf, 0, sizeof(buf));
				if( (len = read(fd, buf, sizeof(buf))) <= 0 )
				{
						 close(fd);
						 return -4;
				}

				if( (ptr = strstr(buf, "t=")) == NULL) 
				{
			 			return -5;
				}
			
				ptr += 2;
				*temp = atof(ptr)/1000;
				
				return 0;
			}
		}
	}

	fclose(fp);
	return -6;
}




int get_custom_serial(char *serial_number, int buffer_size)
{
		char 				first_device[64] = {0};
		FILE				 *fp = NULL;
		char 				line[256] = {0};
		DIR 				*dirp = NULL;
		struct 				dirent *direntp = NULL;
		char 				physical_serial[64] = {0};
		int 				written = 0;
		static int 			counter = 1;
		const char 			*config_file = "/home/iot25/yangjiayu/Get-message/etc/ds18b20.conf";
		const int 			min_serial_size = 12;

		
		if (serial_number == NULL || buffer_size < min_serial_size) 
		{
			     return -1;
		}

		// 如果配置文件存在，直接读取
		fp = fopen(config_file, "r");
		if(fp != NULL) 
		{
				while( (fgets(line, sizeof(line), fp)) != NULL ) 
				{
						if(sscanf(line, "%s", serial_number) == 1)
						{
								fclose(fp);		
								return 0;
						}
				}

				fclose(fp);
		}

		// 否则自动检测第一个DS18B20设备并创建配置
		if((dirp = opendir("/sys/bus/w1/devices/")) == NULL) 
		{
				return -2;
		}

		while((direntp = readdir(dirp)) != NULL) 
		{
				if(strstr(direntp->d_name, "28-"))
				{
						strncpy(physical_serial, direntp->d_name, sizeof(physical_serial)-1);
						break;
				}	
		}
	
		closedir(dirp);

		if(physical_serial[0] == '\0') 
		{
				return -3;
		}

		// 生成自定义序列号 (例如: DS18B20_001)
		if ( snprintf(serial_number, buffer_size, "DS18B20_%03d", counter++) < 0 )
		{
				return -4;
		}

		printf("%s\n",serial_number);
		// 保存到配置文件
		if( (fp = fopen(config_file, "w")) == NULL )
		{
				return -5;
		}
	
		fprintf(fp, "%s %s\n", serial_number, physical_serial);
		fclose(fp);

		return 0;
}




