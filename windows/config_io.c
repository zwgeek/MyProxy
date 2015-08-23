#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include "logger.h"
#include "prot_fword.h"
#define FILE_PATH "config.ini"

/**
 * used : read config file
 * author : zhanghengwei
 * 将端口映射存入一张表中
 */

//全局变量，获得所有的配置信息
Cfg cfg[10];
//全局计数器
int num;

//获取所有的端口映射
//logger有点问题：执行的时候还没初始化
int getcfgs(){
	FILE *fp;	
	char protocol[15] ;
	char prot[5] ;
	num = -1;
	fp = fopen(FILE_PATH, "rt");
	if(fp == NULL){
		fprintf(logger, "Cannot open file strike any key exit!\n\r");
		return 0;
	}
	while(fgets(protocol, 15, fp) != NULL){
		num++;
		fgets(prot, 5, fp);
		strcpy(cfg[num].prot, prot);
		strcpy(cfg[num].protocol, protocol);
	}
	fclose(fp);
	return 1;
}

//获取指定协议的端口
int cfgs_getprot(char protocol[]){
	int i;
	for(i=0; i<=num; i++)
		if(!strcmp(cfg[i].protocol, protocol))
			break;
	return atoi(cfg[i].prot);
}

//获取指定协议的端口（字符版）
int cfg_getprot(char protocol){
	int i;
	for(i=0; i<=num; i++)
		if(cfg[i].protocol[0] == protocol)
			break;
	return atoi(cfg[i].prot);
}
