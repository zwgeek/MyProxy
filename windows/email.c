#include <windows.h>
#include <stdio.h>
#include <WinSock.h>
#include <string.h>
#include "logger.h"
#pragma comment(lib, "ws2_32.lib")

#define SMTP_SERVER "smtp.exmail.qq.com"
#define SMTP_PROT 25
#define EMAIL_HOST "zhanghengwei@zhiping.tv"
#define EMAIL_PW "zhang201018"
#define EMAIL_DST "zhanghwmail163@163.com"
#define EMAIL_CONTENTS "Subject: PortFword报警\r\n\r\n"
/**
 * this is a send email file
 * author : zhanghengwei
 */

typedef struct{
	unsigned int d4:6;
	unsigned int d3:6;
	unsigned int d2:6;
	unsigned int d1:6;
} Base64Date6;

//协议中加密部分使用的是base64方法
char ConvertToBase64(char uc);
void EncodeBase64(char *dbuf, char *buf128, int len);
void SendMail(char *email, char *Message);
int  OpenSocket(struct sockaddr *addr);

int main(){
	SendMail(EMAIL_DST, "Message");
	return 0;
}

char ConvertToBase64(char uc){
	if(uc < 26)
		return 'A' + uc;
	if(uc < 52)
		return 'a' + (uc - 26);
	if(uc < 62)
		return '0' + (uc-52);
	if(uc == 62)
		return '+';
	return '/';
}

//base64的实现
void EncodeBase64(char *dbuf, char *buf128, int len){
	Base64Date6 *bd6 = NULL;
	int i;
	char buf[256] = {0};
	char *tmp = NULL;
	char swap = '\0';

	memset(buf, 0, 256);
	strcpy(buf, buf128);
	for(i = 1; i <= len/3; i++){
		tmp    			=  buf + (i-1)*3;
		swap   			=  tmp[2];
		tmp[2] 			=  tmp[0];
		tmp[0] 			=  swap;
		bd6    			=  (Base64Date6 *)tmp;
		dbuf[(i-1)*4+0] = ConvertToBase64((unsigned int)bd6->d1);
    	dbuf[(i-1)*4+1] = ConvertToBase64((unsigned int)bd6->d2);
    	dbuf[(i-1)*4+2] = ConvertToBase64((unsigned int)bd6->d3);
    	dbuf[(i-1)*4+3] = ConvertToBase64((unsigned int)bd6->d4);
	}
	if (len % 3 == 1){
		tmp             = buf + (i-1)*3;
    	swap            = tmp[2];
    	tmp[2]          = tmp[0];
    	tmp[0]          = swap;
    	bd6             = (Base64Date6 *)tmp;
    	dbuf[(i-1)*4+0] = ConvertToBase64((unsigned int)bd6->d1);
    	dbuf[(i-1)*4+1] = ConvertToBase64((unsigned int)bd6->d2);
    	dbuf[(i-1)*4+2] = '=';
    	dbuf[(i-1)*4+3] = '=';
	} 
	if(len%3 == 2){
	    tmp             = buf+(i-1)*3;
	    swap            = tmp[2];
	    tmp[2]          = tmp[0];
	    tmp[0]          = swap;
	    bd6             = (Base64Date6 *)tmp;
	    dbuf[(i-1)*4+0] = ConvertToBase64((unsigned int)bd6->d1);
	    dbuf[(i-1)*4+1] = ConvertToBase64((unsigned int)bd6->d2);
	    dbuf[(i-1)*4+2] = ConvertToBase64((unsigned int)bd6->d3);
	    dbuf[(i-1)*4+3] = '=';
	}
	return;
}

//发送邮件
void SendMail(char *email, char *Message){
	int  sockfd     = 0;
	char buf[1500]  = {0};
	char rbuf[1500] = {0};
	char login[128] = {0};
	char pass[128]  = {0};
  char body[50]   = EMAIL_CONTENTS;
  struct sockaddr_in their_addr = {0};
  struct hostent *hptr;
  WSADATA WSAData;

  strcat(body, Message); 

  //Init Windows Socket  使用宏
	if (WSAStartup(MAKEWORD(2,2), &WSAData) != 0){
		fprintf(logger, "Init Windows Socket Failed::%s\n",GetLastError());
		return;
	}
  	memset(&their_addr, 0, sizeof(their_addr));
  
  	their_addr.sin_family = AF_INET;
  	their_addr.sin_port   = htons(SMTP_PROT);
  	hptr                  = gethostbyname(SMTP_SERVER); 
  	memcpy(&their_addr.sin_addr.S_un.S_addr, hptr->h_addr_list[0], hptr->h_length); 
  	fprintf(logger, "IP of %s is : %d:%d:%d:%d\n",
  		SMTP_SERVER, 
        their_addr.sin_addr.S_un.S_un_b.s_b1, 
        their_addr.sin_addr.S_un.S_un_b.s_b2, 
        their_addr.sin_addr.S_un.S_un_b.s_b3, 
        their_addr.sin_addr.S_un.S_un_b.s_b4);

  	//连接邮件服务器，如果连接后没有响应，则2秒后重新连接
  	sockfd = OpenSocket((struct sockaddr *)&their_addr);
  	memset(rbuf, 0, 1500);
  	while(recv(sockfd, rbuf, 1500, 0) == 0){
    	fprintf(logger, "reconnect...\n");
    	Sleep(2);
    	sockfd = OpenSocket((struct sockaddr *)&their_addr);
    	memset(rbuf, 0, 1500);
  	}
  	fprintf(logger, "RBUF: %s\n", rbuf);

  	// EHLO
  	memset(buf, 0, 1500);
  	sprintf(buf, "EHLO HYL-PC\r\n");
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "EHLO REceive: %s\n", rbuf);

  	// AUTH LOGIN
  	memset(buf, 0, 1500);
  	sprintf(buf, "AUTH LOGIN\r\n");
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "Auth Login Receive: %s\n", rbuf);

  	// USER
  	memset(buf, 0, 1500);
  	sprintf(buf, EMAIL_HOST);//你的邮箱账号
  	memset(login, 0, 128);
  	EncodeBase64(login, buf, strlen(buf));
  	sprintf(buf, "%s\r\n", login);
  	send(sockfd, buf, strlen(buf), 0);
  	fprintf(logger, "Base64 UserName: %s\n", buf);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "User Login Receive: %s\n", rbuf);

  	// PASSWORD
  	sprintf(buf, EMAIL_PW);//你的邮箱密码
  	memset(pass, 0, 128);
  	EncodeBase64(pass, buf, strlen(buf));
  	sprintf(buf, "%s\r\n", pass);
  	send(sockfd, buf, strlen(buf), 0);
  	fprintf(logger, "Base64 Password: %s\n", buf);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "Send Password Receive: %s\n", rbuf);
  	
  	// MAIL FROM
  	memset(buf, 0, 1500);
  	sprintf(buf, "MAIL FROM: %s\r\n", EMAIL_HOST);
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "set Mail From Receive: %s\n", rbuf);
  	
  	// RCPT TO 第一个收件人
  	sprintf(buf, "RCPT TO:<%s>\r\n", email);
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "Tell Sendto Receive: %s\n", rbuf);

  	// DATA 准备开始发送邮件内容
  	sprintf(buf, "DATA\r\n");
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "Send Mail Prepare Receive: %s\n", rbuf);
  	
  	// 发送邮件内容，\r\n.\r\n内容结束标记
  	sprintf(buf, "%s\r\n.\r\n", body);
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "Send Mail Receive: %s\n", rbuf);
  	
  	// QUIT
  	sprintf(buf, "QUIT\r\n");
  	send(sockfd, buf, strlen(buf), 0);
  	memset(rbuf, 0, 1500);
  	recv(sockfd, rbuf, 1500, 0);
  	fprintf(logger, "Quit Receive: %s\n", rbuf);
  	//清理工作
  	closesocket(sockfd);
  	WSACleanup();
  	return;
}

// 打开TCP Socket连接
int OpenSocket(struct sockaddr *addr)
{
	int sockfd = 0;
  	sockfd=socket(PF_INET, SOCK_STREAM, 0);
  	if(sockfd < 0){
    	fprintf(logger, "Open sockfd(TCP) error!\n");
    	exit(-1);
  	}
  	if(connect(sockfd, addr, sizeof(struct sockaddr)) < 0){
    	fprintf(logger, "Connect sockfd(TCP) error!\n");
    	exit(-1);
  	}
  	return sockfd;
}


