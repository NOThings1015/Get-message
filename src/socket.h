/********************************************************************************
 *      Copyright:  (C) 2025 LingYun<iot25@lingyun>
 *                  All rights reserved.
 *
 *       Filename:  socket.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(11/04/25)
 *         Author:  LingYun <iot25@lingyun>
 *      ChangeLog:  1, Release initial version on "11/04/25 17:00:19"
 *                 
 ********************************************************************************/
#ifndef SOCKET_H_

#define SOCKET_H_

extern int socket_server_init(char *listen_ip,int listen_port); //服务器端定义

extern int socket_client_init(const char *server_ip, int server_port, struct sockaddr_in *serv_addr); //客户端定义

extern void Print_Client_Usage(char *progname); //客户端使用方法

extern void Print_Server_Usage(char *progname); //服务器使用方法

#endif

