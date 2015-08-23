/**
 * this is a prot_fword.h
 * author : zhanghengwei
 */

//是否定义
#ifndef PROTFWORD_FLAG
	#define TRUE 1
	#define FALSE 0
	typedef struct{
		char protocol[15];
		char prot[5];
	}Cfg, *Cfgs;

	//config.io
	int getcfgs();
	int cfgs_getprot(char[]);
	int cfg_getprot(char);
#endif