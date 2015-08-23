#include "config_io.h"
#include "prot_fword.h"

//hide console
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") 
/**
 * this is main.c
 * author: zhanghengwei
 */

int main(int argc, char *argv[]){
	int ret;
	ret = ProtFword();
	return ret;
}