/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  sqlite.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(09/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "09/04/25 16:33:31"
 *                 
 ********************************************************************************/

#include <sqlite3.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sqlite.h"

/*
int main()
{
	char	sqlite_path[200]="/home/iot25/yangjiayu/Get-message/sqlite3/Temp.db";
    char    message[20]="Hello!!!";

	char*	table_name = "TempData";

	char    output_file[128] = "/home/iot25/yangjiayu/Get-message/tmp/output_file.txt";
	int     i=0;

	sqlite3 *db;

	db = sqlite_open(sqlite_path);
	create_table(db);
	
	for(i=0;i<6;i++)
	{
		sqlite_write(db,message);

	}
	
	sqlite_read(db, output_file);

	
	int fd = STDOUT_FILENO; // 使用标准输出文件描述符

	// 调用封装函数
	if (sqlite_read_1st(db, fd) == 0)
	{
		printf("Data successfully read and written to file descriptor.\n");

		delete_1st_row(db, table_name);
	}

	else 
	{
		fprintf(stderr, "Failed to read data from database.\n");
	}


//	sqlite_clear(sqlite_path);
	return 0;
}
*/


//时间戳去重机制
//问题原因：
//若客户端断线期间多次写入相同数据，服务器无法识别重复消息。
sqlite3 *sqlite_open(char *sqlite_path)
{
		sqlite3 *db;
    	char *errMsg = NULL;
    	int rc;

    	rc = sqlite3_open(sqlite_path, &db);
    	if (rc) 
		{
    		    fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        		sqlite3_close(db);
       			return NULL;
    	}

		return db;
}

int create_table(sqlite3 *db) 
{
    char *errMsg = NULL;
    int rc;

    const char *createTableSQL = "CREATE TABLE IF NOT EXISTS TempData ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
								"timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "  // 新增时间戳字段
                                "data TEXT NOT NULL);";

    rc = sqlite3_exec(db, createTableSQL, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) 
	{
	        fprintf(stderr, "SQL error: %s\n", errMsg);
    	    sqlite3_free(errMsg);
        	sqlite3_close(db);
       		return -2;
    }

    return 0;
}


int	sqlite_write(sqlite3 *db, char *message)
{
	char 			*errMsg = NULL;
	int 			rc=-1;

	// 插入临时数据（使用参数化查询）
	const char *insertSQL = "INSERT INTO TempData (data) VALUES (?);";
	sqlite3_stmt *stmt;
	rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, NULL);
	if (rc != SQLITE_OK) 
	{
			fprintf(stderr, "SQL error: %s\n", errMsg);
			sqlite3_free(errMsg);
			return -2;
	}
	
	// 绑定参数
	rc = sqlite3_bind_text(stmt, 1, message, -1, SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -3;
	}
	
	// 执行插入操作
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -4;									
	}


	printf("######[SUCCESS] Data written to SQLite: %s\n", message);  // 打印插入成功日志
	
	sqlite3_finalize(stmt);
	
	//printf("[INFO] SQLite write operation completed.\n"); // 打印最终操作结果
	return 0;
}


int sqlite_read_1st(sqlite3 *db, int fd) 
{
	char			buf[256] = "";
	char 			*errmsg = NULL;
	char 			**result;
	int 			rows, cols, rc;

	
	const char *selectSQL = 
		"SELECT data FROM TempData ORDER BY timestamp ASC LIMIT 1;";

	// 执行查询
	rc = sqlite3_get_table(db, selectSQL, &result, &rows, &cols, &errmsg);
	if (rc != SQLITE_OK) 
	{
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return -1;
	}


	snprintf(buf,sizeof(buf),"%s\n", result[1]);

	// 释放资源
	sqlite3_free_table(result);

	if( ( write(fd, buf, strlen(buf))) < 0 )
	{
		return -1;
	}

	return 0;
}



int delete_1st_row(sqlite3 *db, char* table_name) 
{
		char		*error_message = NULL;
		int			rc = 0;
		char sql_query[256];	// 构建 DELETE 语句
		
		snprintf(sql_query, sizeof(sql_query), "DELETE FROM %s WHERE id = (SELECT id FROM %s ORDER BY id ASC LIMIT 1);", table_name, table_name);

		rc = sqlite3_exec(db, sql_query, NULL, NULL, &error_message);	// 执行 SQL 语句
		if (rc != SQLITE_OK) 
		{
				fprintf(stderr, "SQL error: %s\n", error_message);
				sqlite3_free(error_message);
				return -1;
		}

		printf("Row deleted successfully.\n");
		return 0;
}



//按时间顺序读取数据
//问题原因：
//默认查询未排序，可能导致数据读取顺序混乱

int	sqlite_read(sqlite3 *db, char *output_file)
{
		char 		*errMsg = NULL;
		int 		rc = -1;
		int 		line = 0;

		// 查询数据
 		// 按时间戳排序查询
		const char *selectSQL = "SELECT data FROM TempData ORDER BY timestamp ASC;";
		sqlite3_stmt *stmt;
		rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, NULL);
		if (rc != SQLITE_OK) 
		{
				fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
				return -2;
		}


		//"w" 模式会覆盖原有文件内容，导致每次运行都清空文件。
		//"a" 表示追加，可保留历史数据
		FILE *fp = fopen(output_file, "w");
		if (!fp) 
		{
				fprintf(stderr, "Failed to open output file: %s\n", strerror(errno));
				sqlite3_finalize(stmt);
				return -3;
		}
	
		while (sqlite3_step(stmt) == SQLITE_ROW) 
		{
				const unsigned char *text = sqlite3_column_text(stmt, 0);
				if (text) 
				{
						fprintf(fp, "%s\n", text);
				}
			
		}
	     	
		sqlite3_finalize(stmt);
		fclose(fp);

		return 0;
			
}


int sqlite_clear(sqlite3 *db)
{
		char *errMsg = NULL;
		int rc;

		const char *deleteSQL = "DELETE FROM TempData;"; // 删除表中所有数据
		rc = sqlite3_exec(db, deleteSQL, NULL, NULL, &errMsg);
		if (rc != SQLITE_OK)
		{
				fprintf(stderr, "SQL error: %s\n", errMsg);
				sqlite3_free(errMsg);
				return -2;
		}

		printf("[SUCCESS] 成功清空数据库中的临时数据\n");
		return 0;
}



int is_database_empty(sqlite3 *db) 
{
		sqlite3_stmt 		*stmt = NULL;
		int 				rc = -1;
		int					result = -1;

		
		//  检查 TempData 表是否存在
		const char *checkTableSQL = 
			        "SELECT COUNT(*) FROM sqlite_master "
					"WHERE type='table' AND name='TempData';";
		rc = sqlite3_prepare_v2(db, checkTableSQL, -1, &stmt, NULL);

		if (rc != SQLITE_OK) 
		{
			    fprintf(stderr, "SQL错误: %s\n", sqlite3_errmsg(db));
				goto cleanup;
		}
		
		int table_exists = 0;
		if (sqlite3_step(stmt) == SQLITE_ROW) 
		{
				        
			table_exists = sqlite3_column_int(stmt, 0);
				
		}
			    
		sqlite3_finalize(stmt);
		stmt = NULL;			    
		if (!table_exists)  //// 表不存在，视为空数据库
		{
			result = 1;        
			goto cleanup;
					
		}

		const char *countSQL = "SELECT COUNT(*) FROM TempData;";

		    
		rc = sqlite3_prepare_v2(db, countSQL, -1, &stmt, NULL);
			  
		if (rc != SQLITE_OK) 
		{
				
			fprintf(stderr, "SQL错误: %s\n", sqlite3_errmsg(db));
				
			goto cleanup;
				
		}

			
		int row_count = 0;
			
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			row_count = sqlite3_column_int(stmt, 0);
		}
		 
		result = (row_count == 0) ? 1 : 0;
cleanup:   
		if (stmt) sqlite3_finalize(stmt);
		return result;
}



