/********************************************************************************
 *      Copyright:  (C) 2025 YANG Studio
 *                  All rights reserved.
 *
 *       Filename:  packet.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(31/05/25)
 *         Author:  YANG JIAYU <yangjiayu@gmail.com>
 *      ChangeLog:  1, Release initial version on "31/05/25 12:27:27"
 *                 
 ********************************************************************************/
#ifndef __PACKET_H_
#define __PACKET_H_

#include <signal.h>
#include <time.h>

#define DEVID_LEN          8

typedef struct pack_info_s
{
	char          devid[DEVID_LEN+1];  	/*  device ID  */
	struct tm     sample_time;     		/*  sample time  */
	float         temper;            	/*  sample temperature */
} pack_info_t;

extern int get_serial_number(char *serial_number, int size, int sn);

extern int get_time(char *buffer, int buf_size);


#endif
