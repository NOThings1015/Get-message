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

#define DEVID_LEN          13

typedef struct pack_info_s
{
	char          devid[DEVID_LEN+1];  	/*  device ID  */
	struct tm     sample_time;     		/*  sample time  */
	float         temper;            	/*  sample temperature */
} pack_info_t;


typedef int (* pack_proc_t)(pack_info_t *pack_info, uint8_t *pack_buf, int size);//typedef 返回值类型 (*类型名)(参数列表)

extern int get_serial_number(char *serial_number, int size, int sn);

extern int get_time(struct tm *now_time);

extern int packet_segmented_pack(pack_info_t *pack_info, uint8_t *pack_buf, int size);

extern int packet_json_pack(pack_info_t *pack_info, uint8_t *pack_buf, int size);

extern int packet_tlv_pack(pack_info_t *pack_info, uint8_t *pack_buf, int size);

extern int packet_tlv_unpack(const uint8_t *pack_buf, int size, pack_info_t *pack_info);
#endif
