/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  sqlite.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(10/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "10/04/25 07:42:10"
 *                 
 ********************************************************************************/
#ifndef	SQLITE_H_

#include <sqlite3.h>
#define	SQLITE_H_

extern 	int sqlite_write(char *sqlite_path, char *message);	//写入数据到指定的数据库
extern	int sqlite_read(char *sqlite_path, char *output_file);//读取指定数据库中的数据,将数据写入文件
extern	int sqlite_clear(char *sqlite_path);   				//清除指定数据库中的数据
extern  int create_table(char *sqlite_path);				//指定路径创建sqlite数据库表格
extern  int is_database_empty(char *sqlite_path);			//检查数据是否非空
extern	int sqlite_read_1st(char *sqlite_path, int fd);
extern 	int delete_1st_row(char* db_path, char* table_name);  
#endif
