//this is a windows tcp swap
#include <stdio.h>
#include <winsock2.h>
#include "logger.h"
#include "prot_fword.h"
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

/**
 * system : windows
 * author : zhanghengwei
 * 此环境下io逻辑比较复杂使用Thread做异步
 */

//将DataBuf中的DataLen个字节发到s去
int DataSend(SOCKET sock, char *DataBuf, int DataLen){
	int nBytesLeft = DataLen;
	int nBytesSent = 0;
	int Ret;
	//set socket to blocking mode
 	int iMode = 0;
 	ioctlsocket(sock, FIONBIO, (u_long FAR*) &iMode);

 	while(nBytesLeft > 0){
 		Ret = send(sock, DataBuf+nBytesSent, nBytesLeft, 0);
 		if(Ret <= 0)
 			break;
 		nBytesSent += Ret;
 		nBytesLeft -= Ret;
 	}
 	return nBytesSent;
}

//Mapping Port Proc
DWORD WINAPI MappingPort(LPVOID lpParameter){
	SOCKET MapSrcSocket = ((SOCKET*)lpParameter)[0];
	SOCKET MapDstSocket = ((SOCKET*)lpParameter)[1];
	int Ret = 0, nRecv;
	fd_set Fd_Read;
	char RecvBuffer[BUF_SIZE];

	while(TRUE){
		FD_ZERO(&Fd_Read);
		FD_SET(MapSrcSocket, &Fd_Read);
		FD_SET(MapDstSocket, &Fd_Read);
		Ret = select(0, &Fd_Read, NULL, NULL, NULL);
		if(Ret <= 0){
			fprintf(logger, "Init Select API Failed\n");
			goto error;
		}
		if(FD_ISSET(MapSrcSocket, &Fd_Read)){
			nRecv = recv(MapSrcSocket, RecvBuffer, sizeof(RecvBuffer), 0);
			if(nRecv <= 0){
				fprintf(logger, "MapSrcSocket Recv Failed\n");
				goto error;
			}
			Ret = DataSend(MapDstSocket, RecvBuffer, nRecv);
			if(Ret == 0 || Ret != nRecv){
				fprintf(logger, "MapSrcSocket Send Failed\n");
				goto error;
			}
		}
		if(FD_ISSET(MapDstSocket, &Fd_Read)){
			nRecv = recv(MapDstSocket, RecvBuffer, sizeof(RecvBuffer), 0);
			if(nRecv <= 0){
				fprintf(logger, "MapDstSocket Recv Failed\n");
				goto error;
			}
			Ret = DataSend(MapSrcSocket, RecvBuffer, nRecv);
			if(Ret == 0 || Ret != nRecv){
				fprintf(logger, "MapDstSocket Send Failed\n");
				goto error;
			}
		}
	} //end while
error:
	closesocket(MapSrcSocket);
	closesocket(MapDstSocket);
	return 0;
}

//main
int main(int argc, char *argv[]){
	WSADATA Ws;
	SOCKET MainSocket, 	MapSrcSocket, MapDstSocket;
	SOCKET Sockets[2];
	struct sockaddr_in MainAddr, MapSrcAddr, MapDstAddr;
	int Ret = 0, nRecv;
	int AddrLen = 0;
	int nNetTimeout = 1000;
	char Message[BUF_SIZE] = {0};
	int DstPort;
	HANDLE hThread = NULL;

	//Init config.ini
	getcfgs();
	
	//Init logger
	logger = fopen(LOGGER_PATH, "at");

	//Init Windows Socket  使用宏
	if (WSAStartup(MAKEWORD(2,2), &Ws) != 0){
		fprintf(logger, "Init Windows Socket Failed::%s\n",GetLastError());
		return -1;
	}

	// Init Windows sockaddr_in
	MainAddr.sin_family = AF_INET;
	MainAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	MainAddr.sin_port = htons(22);
	memset(MainAddr.sin_zero, 0x00, 8);

	//Create Socket
	/**
	 * af: 创建TCP或UDP的套接字是使用AF_INET
	 * type: SOCK_STREAM、SOCK_DGRAM和SOCK_RAM分别表示数据流，数据包，原始套接字
	 * protocol :对于SOCK_STREAM套接字类型，该字段可以为IPPROTO_TCP或0。对于SOCK_DGRAM套接字类型，该字段为IPPROTO_UDP或0。
	 */
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (MainSocket == INVALID_SOCKET){
		fprintf(logger, "Create MainSocket Failed::%s\n", GetLastError());
		return -1;
	}

	//Bind Socket SOCKET_ERROR:0
	Ret = bind(MainSocket, (struct sockaddr*)&MainAddr, sizeof(MainAddr));
	if(Ret == SOCKET_ERROR){
		fprintf(logger, "Bind MainSocket Failed::%s\n",GetLastError());
		return -1;
	}

	//Listen
	Ret = listen(MainSocket, 20);
	if (Ret == SOCKET_ERROR){
		fprintf(logger, "Listen MainSocket Failed::%s\n",GetLastError());
		return -1;
	}

	fprintf(logger, "The Mapping-Server is starting\n");

	//running
	while(TRUE){
		AddrLen = sizeof(MapSrcAddr);
		MapSrcSocket = accept(MainSocket, (struct sockaddr*)&MapSrcAddr, &AddrLen);
		if (MapSrcSocket == INVALID_SOCKET){
			fprintf(logger, "Accept Failed::%s\n",GetLastError());
			break;
		}
		Sockets[0] = MapSrcSocket;

		//Decide the Port and Init DstPort
		//EWOULDBLOCK, EAGAIN ?
		setsockopt(MapSrcSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&nNetTimeout,sizeof(int));
		nRecv = recv(MapSrcSocket, Message, sizeof(Message), 0);
		if(nRecv <= 0){
			if(nRecv == -1){
				fprintf(logger, "Init DstPort : 23\n");
				DstPort = 23;
			}else{
				fprintf(logger, "MapSrcSocket Recv Failed\n");
				break;
			}
		}else{
			fprintf(logger, "Init DstPort\n");
			DstPort = cfg_getprot(*Message);
		}
					
		//Create MapDstSocket
		MapDstSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(MapDstSocket == INVALID_SOCKET){
			fprintf(logger, "Create MapDstSocket Failed::%s\n", GetLastError());
			break;
		}
		//Windows sockaddr_in
		MapDstAddr.sin_family = AF_INET;
		MapDstAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		MapDstAddr.sin_port = htons(DstPort);
		memset(MapDstAddr.sin_zero, 0x00, 8);
		//Connect to Dst
		Ret = connect(MapDstSocket, (struct sockaddr*)&MapDstAddr, sizeof(MapDstAddr));
		if(Ret == SOCKET_ERROR){
			fprintf(logger, "Connect Error::%s::\n", GetLastError());
			break;
		}

		//First Send Message
		if(strlen(Message) != 0){
			Ret = DataSend(MapDstSocket, Message, nRecv);
			if(Ret == 0 || Ret != nRecv){
				fprintf(logger, "MapSrcSocket Send Failed\n");
				break;
			}
		}

		Sockets[1] = MapDstSocket;
		hThread = CreateThread(NULL, 0, MappingPort, (LPVOID)Sockets, 0, NULL);
		if(hThread == NULL){
			fprintf(logger, "Create Thread Failed!\n");
			break;
		}

		//clear Message
		memset(Message, 0x00, BUF_SIZE);
	}
	//close
	closesocket(MainSocket);
	WSACleanup();

	return 0;
}





















