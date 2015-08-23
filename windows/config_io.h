#ifndef CONFIGIO_FLAG
	#define CONFIGIO_FLAG 1
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