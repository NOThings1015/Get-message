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
#include <stdint.h>
#include <arpa/inet.h> 

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


int get_time(struct tm *now_time) 
{
	time_t 				rawtime;

	//获取当前时间（从1970年1月1日至今的秒数）
	time(&rawtime);

	// 将时间转换为本地时间
	localtime_r(&rawtime, now_time);

	return 0;
}


int packet_segmented_pack(pack_info_t *pack_info, uint8_t *pack_buf, int size)
{
	char              strtime[23] = "";
	struct tm        *ptm;
	char             *buf = (char *)pack_buf;	

	if( !pack_info || !buf || size<=0 )
	{
		log_error("Invalid input arguments\n");
		return -1;
	}

	ptm = &pack_info->sample_time;
	// 使用 strftime()打印格式化后的时间
	strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", ptm);

	memset(buf, 0, size);
	snprintf(buf, size, "%s|%s|%.3f", pack_info->devid, strtime, pack_info->temper);

	return strlen(buf);
}



int packet_json_pack(pack_info_t *pack_info, uint8_t *pack_buf, int size)
{
	char              strtime[23] = {'\0'};
	struct tm        *ptm;
	char             *buf = (char *)pack_buf;

	if( !pack_info || !buf || size<=0 )
	{
		log_error("Invalid input arguments\n");
		return -1;
	}

	ptm = &pack_info->sample_time;
	strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", ptm);

	memset(buf, 0, size);
	snprintf(buf, size, "{\"devid\":\"%s\", \"time\":\"%s\",\"temperature\":\"%.3f\"}",
			pack_info->devid, strtime, pack_info->temper);

	return strlen(buf);
}



#define CRC16_ITU_T_POLY 0x1021   /*  Define the CRC-ITU-T polynomial */
static uint16_t crc_itu_t(const uint8_t *data, size_t length)
{
	uint16_t          crc = 0xFFFF;
	size_t            i, j;

	for (i=0; i<length; i++)
	{
		crc ^= ((uint16_t)data[i] << 8);

		for(j=0; j<8; j++)
		{
			if (crc & 0x8000)	//二进制模2除法,最高位为1时够除
			{
				crc = (crc << 1) ^ CRC16_ITU_T_POLY;	//异或（相当于减法）
			}
			else
			{
				crc <<= 1;
			}
		}
	}

	return crc;
}


uint16_t to_big_endian(uint16_t num)
{
	    return (num << 8) | (num >> 8);
}





#define TLV_HEADER          0xFCCD


#define TLV_MINSIZE         7

/*  TVL Tag definition */
enum
{
	TAG_TEMPERATURE = 1,
	TAG_HUMIDITY,
	TAG_NOISY,
};


int packet_tlv_pack(pack_info_t *pack_info, uint8_t *pack_buf, int size)
{
	struct tm          *ptm;
	int                 ofset = 0;
	uint16_t            crc;

	if( !pack_info || !pack_buf || size<TLV_MINSIZE )

	{
		log_error("Invalid input arguments\n");
		return -1;
	}

	 /*****|    TLV Header(2B)    |*****/
	*(uint16_t *)pack_buf = to_big_endian(TLV_HEADER);
	ofset += 2;

	/*****|      TLV Tag(1B)     |*****/
	pack_buf[ofset++] = TAG_TEMPERATURE;

	/*  Skip data length here, we will calculate it later */
	ofset += 2;

	/*****|  payload data value  |*****/

	/*  6 bytes sample time */
	ptm = &pack_info->sample_time;
	pack_buf[ofset++] = (uint8_t)(ptm->tm_year+1900-2000);
	pack_buf[ofset++] = (uint8_t)(ptm->tm_mon+1);
	pack_buf[ofset++] = (uint8_t)(ptm->tm_mday);
	pack_buf[ofset++] = (uint8_t)(ptm->tm_hour);
	pack_buf[ofset++] = (uint8_t)(ptm->tm_min);
	pack_buf[ofset++] = (uint8_t)(ptm->tm_sec);

	/*   bytes device SN */
	strncpy((char *)(pack_buf+ofset), pack_info->devid, DEVID_LEN);
	ofset += DEVID_LEN;

	/*  2 bytes temperature value */
	pack_buf[ofset++] = (int)pack_info->temper;  /*  integer part */
	pack_buf[ofset++] = (((int)(pack_info->temper*100))%100); /*  fractional part */

	/*****|    TLV Length(2B)    |*****/
	*(uint16_t *)(pack_buf+3) = (ofset-5);

	/*****|      TLV CRC(2B)     |*****/
	crc = crc_itu_t(pack_buf, ofset);
	
	crc = htons(crc); //转换为网络字节序
	*(uint16_t *)(pack_buf+ofset) = crc;
	ofset += 2;

	// 打印 pack_buf 的内容
	printf("packet sample data:");
	for (int i = 0; i < ofset; i++)
	{
		printf("%02x ", pack_buf[i]);
	}
	printf("\n");

	return ofset;

}




int packet_tlv_unpack(const uint8_t *pack_buf, int size, pack_info_t *pack_info) 
{
	uint16_t 			header;

	uint16_t 			received_crc;
	int					offset = 0;
	if (!pack_buf || !pack_info ) 
	{
		fprintf(stderr, "Invalid input arguments\n");
		return -1;
	}

	memcpy(&header, pack_buf, sizeof(header));
	header = ntohs(header); 

	if ( header != TLV_HEADER)
	{
		fprintf(stderr, "Invalid TLV Header\n");
		return -2;
	}

	offset += 2; // Skip TLV Header

	uint8_t tag = pack_buf[offset++];
	if (tag != TAG_TEMPERATURE) 
	{
		fprintf(stderr, "Unsupported TLV Tag\n");
		return -3;
	}


	uint16_t length = *(uint16_t *)(pack_buf + offset);
	offset += 2;


	if (offset + length > size)
	{
		fprintf(stderr, "Buffer size is insufficient\n");
		return -4;
	}

	struct tm sample_time = {0};
	sample_time.tm_year = pack_buf[offset++] + 100; // Year since 2000
	sample_time.tm_mon = pack_buf[offset++] - 1;    // Month (0-11)
	sample_time.tm_mday = pack_buf[offset++];
	sample_time.tm_hour = pack_buf[offset++];
	sample_time.tm_min = pack_buf[offset++];
	sample_time.tm_sec = pack_buf[offset++];
	pack_info->sample_time = sample_time;

	strncpy(pack_info->devid, (const char *)(pack_buf + offset), sizeof(pack_info->devid));
	offset += DEVID_LEN;

	pack_info->temper = pack_buf[offset++];
	pack_info->temper += (pack_buf[offset++] / 100.0);

	// 打印 pack_buf 的内容
	printf("unpack sample data:");
	for (int i = 0; i < offset+2; i++)
	{
		printf("%02x ", pack_buf[i]);
	}
	printf("\n");

	memcpy(&received_crc, pack_buf + offset, sizeof(received_crc));
	received_crc = ntohs(received_crc);  // 网络字节序转主机字节序


	uint16_t crc = crc_itu_t(pack_buf, offset);
	if (	received_crc != crc) 
	{
		fprintf(stderr, "CRC check failed\n");

		printf("%02x,%02x, %02x\n",crc,pack_buf[offset-1],received_crc); 

		return -5;
	}

	return 0;
}





