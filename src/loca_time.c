/*********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  loca_time.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(08/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "08/04/25 14:52:28"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <time.h> // 包含时间处理函数

int get_time(char *buffer, int buf_size) 
{
		//获取当前时间（从1970年1月1日至今的秒数）
		time_t rawtime;
		time(&rawtime);
	
		// 将时间转换为本地时间
		struct tm *timeinfo;
		timeinfo = localtime(&rawtime);
		
		// 打印原始时间（秒数）
		//printf("当前时间的秒数（从1970年1月1日起）: %ld\n", rawtime);
		
		// 打印格式化后的时间
		// acstime()可将struct tm 结构体转换为一个可读的字符串格式
		//printf("当前本地时间: %s", asctime(timeinfo));
		
		// 使用 strftime()支持自定义时间格式
		
		strftime(buffer, buf_size, "%Y-%m-%d %H:%M:%S", timeinfo);
		
		if(buffer < 0)
		{
			printf("Get time error\n");
		}

		//printf("%s\n", buffer);
	
		return 0;

}

