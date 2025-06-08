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
#define	SQLITE_H_
#include <sqlite3.h>

extern 	int sqlite_write(sqlite3 *cli_db, void *pack, int size);

extern	int sqlite_check_read(sqlite3 *cli_db, void *pack, int size, int *bytes);

extern	int delete_1st_row(sqlite3 *cli_db);

extern	int sqlite_clear(sqlite3 *db);   				//清除指定数据库中的数据

extern  int sqlite_init(char *sqlite_path, sqlite3 **cli_db);

extern	void sqlite_terminate(sqlite3 *cli_db);





extern  int is_database_empty(sqlite3 *db);			//检查数据是否非空

extern	int sqlite_read_1st(sqlite3 *db, char *buf, int buf_size);


extern	int sqlite_read(sqlite3 *db, char *output_file);//读取指定数据库中的数据,将数据写入文件



#endif
