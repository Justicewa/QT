#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>		//primitive system data types(包含很多类型重定义，如pid_t、int8_t等)   
#include <sys/socket.h>		//与套接字相关的函数声明和结构体定义，如socket()、bind()、connect()及struct sockaddr的定义等

#include <unistd.h>			//read,write
#include <netinet/in.h>	 	//某些结构体声明、宏定义，如struct sockaddr_in、PROTO_ICMP、INADDR_ANY等
#include <arpa/inet.h>		//某些函数声明，如inet_ntop()、inet_ntoa()等

#include <error.h>			//perror

#include <sys/stat.h>
#include <fcntl.h>
#include "serial.h"


#define MAXLITE 50
#define devpath "/dev/ttyATH0"

typedef struct sockaddr SA;

int main(int argc, char *argv[])
{
	//串口初始化函数"/dev/ttyATH0"
	int fd = serial_init(devpath);
	if (fd < 0){
		perror("serial init:");
	}
	printf("fd = , serial init success\n");
	int opt = -1;
	socklen_t client;
	struct sockaddr_in servaddr, clientaddr;

	int socketfd = socket(PF_INET, SOCK_STREAM, 0);
	if (socketfd < 0){
		perror("fail to socket :");
		exit(-1);
	}
	setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	//端口重用	
	
	bzero(&servaddr, sizeof(servaddr));
//填写网络三元组信息	
/*	servaddr.sin_family 		= PF_INET;*/
/*	servaddr.sin_addr.s_addr 	= inet_addr("127.0.0.1");*/
/*	servaddr.sin_port			= htons(8888);*/
/*
	servaddr.sin_family			= PF_INET;	//TCP 协议			//填充地址信息
	servaddr.sin_addr.s_addr 	= inet_addr(argv[1]);
	servaddr.sin_port 			= htons(atoi(argv[2]));
*/	
	servaddr.sin_family			= PF_INET;
	servaddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	servaddr.sin_port			= htons(2001);

	if (bind(socketfd, (SA*)&servaddr, sizeof(servaddr)) < 0){
		perror("fail to bind");
		exit(-1);
	}
	if (listen(socketfd, 2) < 0){
		perror("fial to listen");
		exit(-1);
	}
	
	while(1){
		printf("wait for a new client\n");
		client = sizeof(clientaddr);
		memset(&clientaddr, 0, sizeof(struct sockaddr_in));
		int connfd = accept(socketfd, (SA *) &clientaddr, &client);
		if (connfd < 0){
			perror("fail to accept");
//			exit(-1);
		} else {
			printf("connection from %s, port %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			while(1){
				unsigned char buff[1024];
				int len = read(connfd, buff, sizeof(buff));//从套接字接收
				if (len < 0){
					perror("read:");
				}else if (len == 0){
					break;
				}else {
					serial_send_data(fd, buff, len);//往串口发
				}
			}
		}
	}
	close(fd);
}
