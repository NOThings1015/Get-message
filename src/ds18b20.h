/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  ds18b20.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(05/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "05/04/25 18:52:04"
 *                 
 ********************************************************************************/

#ifndef	DS18B20_H_
#define DS18B20_H_

extern int  get_ds18b20_tmp(char *serial_number, float *temp);

extern int generate_sensor_message(char *serial_number, char *buffer, int buffer_size);

extern int get_custom_serial(char *serial_number, int buffer_size);
#endif
