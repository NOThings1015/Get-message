/*********************************************************************************
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


int create_table(char *sqlite_path);					//指定路径创建sqlite数据库表格
int sqlite_write(char *sqlite_path, char *message); 	//上传数据存储到指定库文件里面
int sqlite_read(char *sqlite_path, char  *output_file); //读取指定数据库中的数据,将数据写入文件
int sqlite_clear(char *sqlite_path);   					//清除指定数据库中的数据
int is_database_empty(const char *sqlite_path);

//int main()
//{
//	char	sqlite_path[200]="/home/iot25/yangjiayu/Get-message/sqlite3/lalalal.db";
//	char    message[20]="Hello!!!";
//	char    output_file[128] = "/home/iot25/yangjiayu/Get-message/tmp/output_file.txt";
//	int     i=0;
//
//	create_table(sqlite_path);
//	for(i=0;i<6;i++)
//	{
//		sqlite_write(sqlite_path,message);
//
//	}
//	sqlite_read(sqlite_path, output_file);
//}



//时间戳去重机制
//问题原因：
//若客户端断线期间多次写入相同数据，服务器无法识别重复消息。
int create_table(char *sqlite_path) 
{
    sqlite3 *db;
    char *errMsg = NULL;
    int rc;

    rc = sqlite3_open(sqlite_path, &db);
    if (rc) 
	{
    	    fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        	sqlite3_close(db);
       		return -1;
    }

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

    sqlite3_close(db);
    return 0;
}


int	sqlite_write(char *sqlite_path, char *message)
{
	sqlite3 		*db;
	char 			*errMsg = NULL;
	int 			rc=-1;

	// 打开数据库
	rc = sqlite3_open(sqlite_path, &db);
	if (rc) 
	{
			fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return -1;
	}
	
	
	printf("[DEBUG] Trying to write to SQLite: %s\n", message); // 打印待写入的数据

	// 插入临时数据（使用参数化查询）
	const char *insertSQL = "INSERT INTO TempData (data) VALUES (?);";
	sqlite3_stmt *stmt;
	rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, NULL);
	if (rc != SQLITE_OK) 
	{
			fprintf(stderr, "SQL error: %s\n", errMsg);
			sqlite3_free(errMsg);
			sqlite3_close(db);
			return -2;
	}
	
	// 绑定参数
	rc = sqlite3_bind_text(stmt, 1, message, -1, SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			return -3;
	}
	
	// 执行插入操作
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			return -4;									
	}


	printf("[SUCCESS] Data written to SQLite: %s\n", message);  // 打印插入成功日志
	
	sqlite3_finalize(stmt);
	// 关闭数据库
	sqlite3_close(db);

	printf("[INFO] SQLite write operation completed.\n"); // 打印最终操作结果
	return 0;
}


//按时间顺序读取数据
//问题原因：
//默认查询未排序，可能导致数据读取顺序混乱

int	sqlite_read(char *sqlite_path, char *output_file)
{
	sqlite3 	*db;
	char 		*errMsg = NULL;
	int 		rc = -1;
	int 		line = 0;
	// 打开数据库
	rc = sqlite3_open(sqlite_path, &db);
	if (rc) 
	{
			fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return -1;
	}

	// 查询数据
 	// 按时间戳排序查询
	const char *selectSQL = "SELECT data FROM TempData ORDER BY timestamp ASC;";
	sqlite3_stmt *stmt;
	rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, NULL);
	if (rc != SQLITE_OK) 
	{
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return -2;
	}


	//"w" 模式会覆盖原有文件内容，导致每次运行都清空文件。
	//"a" 表示追加，可保留历史数据

	FILE *fp = fopen(output_file, "w");
	if (!fp) 
	{
			fprintf(stderr, "Failed to open output file: %s\n", strerror(errno));
			sqlite3_finalize(stmt);
			sqlite3_close(db);
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
	sqlite3_close(db);
	fclose(fp);

//	if (rc == SQLITE_OK)  //文件写入成功时清理数据库，失败时保留数据供重试 
//	{
//	//		sqlite_clear(sqlite_path);
//	}
	return 0;
			
}


int	sqlite_clear(char *sqlite_path)
{
	sqlite3 	*db;
	char 		*errMsg = NULL;
	int 		rc = -1;

	// 打开数据库
	rc = sqlite3_open(sqlite_path, &db);
	if (rc) 
	{
			fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return -1;
	}

	const char *deleteSQL = "DELETE FROM TempData;";
	rc = sqlite3_exec(db, deleteSQL, NULL, NULL, &errMsg);
	if (rc != SQLITE_OK)
	{
			fprintf(stderr, "SQL error: %s\n", errMsg);
			sqlite3_free(errMsg);
			sqlite3_close(db);
			return -2;
	}

	printf("[SUCCESS] 成功清空数据库 %s 中的临时数据\n", sqlite_path);  // 添加成功日志

	// 关闭数据库
	sqlite3_close(db);
	return 0;	
}




int is_database_empty(const char *sqlite_path) 
{
		sqlite3 *db = NULL;
		sqlite3_stmt *stmt = NULL;
		int rc, result = -1;

		rc = sqlite3_open(sqlite_path, &db);
		if (rc != SQLITE_OK) 
		{
				fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
				return -1;
	    }
		

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
		if (db) sqlite3_close(db);
		return result;
}



