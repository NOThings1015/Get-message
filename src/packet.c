/*********************************************************************************
 *      Copyright:  (C) 2025 YANG Studio
 *                  All rights reserved.
 *
 *       Filename:  packet.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(31/05/25)
 *         Author:  YANG JIAYU <yangjiayu@gmail.com>
 *      ChangeLog:  1, Release initial version on "31/05/25 12:11:46"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "packet.h"
#include "logger.h"
#include "ds18b20.h"


int get_serial_number(char *serial_number, int size, int sn)
{
	if( !serial_number || size<DEVID_LEN )
	{
		log_error("Invalid input arugments\n");
		return -1;
	}

	memset(serial_number, 0, size);
	snprintf(serial_number, size, "RPI3B2505%04d", sn);

	return 0;
}


int get_time(char *buffer, int buf_size) 
{
	time_t rawtime;
	struct tm *timeinfo;

	//获取当前时间（从1970年1月1日至今的秒数）
	time(&rawtime);

	// 将时间转换为本地时间
	timeinfo = localtime(&rawtime);

	// 使用 strftime()打印格式化后的时间
	strftime(buffer, buf_size, "%Y-%m-%d %H:%M:%S", timeinfo);

	if(buffer < 0)
	{   
		printf("Get time error\n");
	}   

	return 0;
}
