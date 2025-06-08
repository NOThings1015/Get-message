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
#include <stdint.h>

#include "sqlite.h"
#include "logger.h"

#define TABLE_NAME "PackTable"	//对当前文件有效,其他文件无法直接使用

int sqlite_init(char *sqlite_path, sqlite3 **cli_db)
{
    	char 					*errmsg = NULL;
    	char					sql[1024] = {0};

		if( !sqlite_path )
		{
			log_error("%s() Invalid input arguments\n", __func__);
			return -1;
		}


		if( 0==access(sqlite_path, F_OK) )
		{
			if( SQLITE_OK != sqlite3_open(sqlite_path, cli_db) )
			{
				log_error("open database file '%s' failure\n", sqlite_path);
				return -2;
			}
			log_info("open database file '%s' ok\n", sqlite_path);
			return 0;
		}


		if( SQLITE_OK != sqlite3_open(sqlite_path, cli_db) )
		{
			log_error("create database file '%s' failure\n", sqlite_path);
			return -2;
		}

		sqlite3_exec(*cli_db, "pragma synchronous = OFF; ", NULL, NULL, NULL);//设置 SQLite 的同步模式为 OFF,禁止同步写入

		sqlite3_exec(*cli_db, "pragma auto_vacuum = 2 ; ", NULL, NULL, NULL);//自动清理，自动回收删除数据后的空间

		snprintf(sql, sizeof(sql), "CREATE TABLE %s(packet BLOB);", TABLE_NAME);
		if( SQLITE_OK != sqlite3_exec(*cli_db, sql, NULL, NULL, &errmsg) )
		{
			log_error("create data_table in database file '%s' failure: %s\n", sqlite_path, errmsg);
			sqlite3_free(errmsg); 		/*  free errmsg  */
			sqlite3_close(*cli_db);   	/*  close databse */
			unlink(sqlite_path);      	/*  remove database file */
			return -3;
		}

    	return 0;
}


void sqlite_terminate(sqlite3 *cli_db)
{
	log_warn("close sqlite database now\n");
	sqlite3_close(cli_db);

	return ;
}


int	sqlite_write(sqlite3 *cli_db, void *pack, int size)
{
	char 					*errmsg = NULL;
	int 					rv = -1;
	sqlite3_stmt			*stat = NULL;
	char					sql[1024] = "";

	if( !pack || size<=0 )
	{
		log_error("%s() Invalid input arguments\n", __func__);
		return -1;
	}

	if( ! cli_db )
	{
		log_error("sqlite database not opened\n");
		return -2;
	}

	snprintf(sql, sizeof(sql), "INSERT INTO %s(packet) VALUES(?)", TABLE_NAME);
	rv = sqlite3_prepare_v2(cli_db, sql, -1, &stat, NULL);	//预编译
	if(SQLITE_OK!=rv || !stat)
	{
		log_error("blob add sqlite3_prepare_v2 failure\n");
		rv = -2;
		goto OUT;
	}

	if( SQLITE_OK != sqlite3_bind_blob(stat, 1, pack, size, NULL) )//绑定参数
	{
		log_error("blob add sqlite3_bind_blob failure\n");
		rv = -3;
		goto OUT;
	}

	rv = sqlite3_step(stat); //执行预编译语句
	if( SQLITE_DONE!=rv && SQLITE_ROW!=rv )
	{
		log_error("blob add sqlite3_step failure\n");
		rv = -4;
		goto OUT;
	}


	printf("write to sqlite: ");
	for (int i = 0; i < size; i++)
	{
		printf("%02x ", ((uint8_t *)pack)[i]);
	}
	printf("\n");


OUT:
	sqlite3_finalize(stat); //释放sqlite3_stmt对象

	if( rv < 0 )
		log_error("add new blob packet into database failure, rv=%d\n", rv);
	else
		log_info("add new blob packet into database ok\n");

	return rv;
}




int sqlite_check_read(sqlite3 *cli_db, void *pack, int size, int *bytes)
{
	char               sql[1024]={0};
	int                rv = 0;
	sqlite3_stmt      *stat = NULL;
	const void        *blob_ptr;

	if( !pack || size<=0 )
	{
		log_error("%s() Invalid input arguments\n", __func__);
		return -1;
	}

	if( ! cli_db )
	{
		log_error("sqlite database not opened\n");
		return -2;
	}

	/*  Only query the first packet record */
	snprintf(sql, sizeof(sql), "SELECT packet FROM %s WHERE rowid = (SELECT rowid FROM %s LIMIT 1);", TABLE_NAME, TABLE_NAME);
	rv = sqlite3_prepare_v2(cli_db, sql, -1, &stat, NULL);
	if(SQLITE_OK!=rv || !stat)
	{
		log_error("firehost sqlite3_prepare_v2 failure\n");
		rv = -3;
		goto out;
	}

	rv = sqlite3_step(stat);//执行预编译语句
	if( SQLITE_DONE!=rv && SQLITE_ROW!=rv ) //SQLITE_ROW：表示查询成功，返回一行数据；SQLITE_DONE：查询成功完成，但没有数据
	{
		log_error("firehost sqlite3_step failure\n");
		rv = -5;
		goto out;
	}

	/*  1rd argument<0> means first segement is packet  */
	blob_ptr = sqlite3_column_blob(stat, 0);	//获取 BLOB 数据的指针
	if( !blob_ptr )
	{
		rv = -6;
		goto out;
	}

	*bytes = sqlite3_column_bytes(stat, 0);		//获取 BLOB bytes大小

	if( *bytes > size )
	{
		log_error("blob packet bytes[%d] larger than bufsize[%d]\n", *bytes, size);
		*bytes = 0;
		rv = -1;
	}

	memcpy(pack, blob_ptr, *bytes);
	rv = 0;

out:
	sqlite3_finalize(stat);
	return rv;
}




int sqlite_read_1st(sqlite3 *db, char *buf, int buf_size) 
{
	char 			*errmsg = NULL;
	char 			**result;
	int 			rows, cols, rc;
	int				len = 0;
	
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

	snprintf(buf, buf_size,"%s", result[1]);
	if( (len < 0) || (len >= buf_size) )
	{
			fprintf(stderr, "Message formatting failed\n");
			return -2;
	}
	// 释放资源
	sqlite3_free_table(result);

	return 0;
}



int delete_1st_row(sqlite3 *cli_db) 
{
	char               sql[1024]={0};
	char              *errmsg = NULL;

	if( ! cli_db )
	{
		log_error("sqlite database not opened\n");
		return -2;
	}

	/*   remove packet from db */
	memset(sql, 0, sizeof(sql));
	snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE rowid = (SELECT rowid FROM %s LIMIT 1);", TABLE_NAME, TABLE_NAME);
	if( SQLITE_OK != sqlite3_exec(cli_db, sql, NULL, 0, &errmsg) )
	{
		log_error("delete first blob packet from database failure: %s\n", errmsg);
		sqlite3_free(errmsg);
		return -2;
	}
	log_warn("delete first blob packet from database ok\n");

	/*   Vacuum the database */
	sqlite3_exec(cli_db, "VACUUM;", NULL, 0, NULL);

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



