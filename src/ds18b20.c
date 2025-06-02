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

#include "logger.h"


int  get_ds18b20_tmp(float *temp)
{
	char            			w1_path[50] = "/sys/bus/w1/devices/";
	char						device_path[50] = "";
	char            			chip[20];
	char            			buf[128];
	DIR            			   *dirp;
	struct dirent  			   *direntp;
	int             			fd =-1;
	char           			   *ptr;
	int             			found = 0;

	//参数检查
	if ( temp == NULL ) 
	{
		return -1;
	}


	if ( (dirp = opendir(w1_path)) == NULL )
	{
		log_error("opendir error: %s\n", strerror(errno));
		return -2;
	}

	while( (direntp = readdir(dirp)) != NULL ) 
	{
		if(strstr(direntp->d_name,"28-"))
		{
			/*  find and get the chipset SN filename */
			strcpy(chip,direntp->d_name);
			found = 1;
			break;
		}
	}

	closedir(dirp);

	if( !found )
	{
		log_error("Can not find ds18b20 in %s\n", w1_path);
		return -3;
	}

	snprintf(device_path, sizeof(device_path), "%s%s/w1_slave", w1_path, chip);

	if( (fd=open(device_path, O_RDONLY)) < 0 )
	{
		log_error("open %s error: %s\n", w1_path, strerror(errno));
		return -4;
	}

	if(read(fd, buf, sizeof(buf)) < 0)
	{
		log_error("read %s error: %s\n", w1_path, strerror(errno));
		return -5;
	}

	ptr = strstr(buf, "t=");
	if( !ptr )
	{
		log_error("ERROR: Can not get temperature\n");
		return -6;
	}

	ptr+=2;

	*temp = atof(ptr)/1000;

	close(fd);

	return 0;
}






