/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  Usage_Method.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(02/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "02/04/25 09:19:47"
 *                 
 ********************************************************************************/
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "usage.h"

void print_usage(char *progname)
{
	printf("%s usage method:\n",progname);
	printf("-p(--port):sepcify serveer listen port.\n");
	printf("-h(--help):printf this help information.\n");
	return ;
}


int usage_message(int argc, char **argv)
{
	int 			port=0;
	int 			ch=0;
	struct option	opts[]={
		{"port", required_argument, NULL, 'p'},		// -p 或 --port，需参数（端口号）
		{"help", no_argument, NULL, 'h'},			// -h 或 --help，无需参数
		{NULL, 0, NULL, 0}							// 结束标记
	};
	while((ch = getopt_long(argc, argv, "p:h", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);		//若选项为 -p/--port，将参数值转换为整数并赋值给 port
				break;
			case 'h':
				print_usage(argv[0]);	//若选项为 -h/--help，调用 print_usage 并返回 0。
				return -1;
		}
	}

	if(!port)
	{
		print_usage(argv[0]);
		return -1;
	}
	return 0;
}
