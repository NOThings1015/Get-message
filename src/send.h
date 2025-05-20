/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  send.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(11/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "11/04/25 12:18:20"
 *                 
 ********************************************************************************/
#ifndef SEND_H_

#define SEND_H_

extern  int send_not_empty(char *sqlite_path, char *output_file, int connfd);
extern  int send_data_from_file(char *output_file, int connfd);

extern	int send_message(int sockfd, const char *data, int length);

#endif
