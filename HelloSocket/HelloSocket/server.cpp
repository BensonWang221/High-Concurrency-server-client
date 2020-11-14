/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           1. First version
09-Nov-2020           2. ①针对Linux在遍历返回的fd_set时，不再每次遍历获取maxFd，而是保存maxFd，在新加或删除时更新maxFd
                         ②注意vector/deque在用iterator遍历时，不能随便erase，注意迭代器陷阱！erase后续iterator失效！！
14-Nov-2020           1. 封装类EasyTcpServer，重写server.cpp

$$HISTORY$$
====================================================================================================*/

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>
	#include <algorithm>
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
	#define strcpy_s strcpy
	#define closesocket close
#endif
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "EasyTcpServer.hpp"


int main()
{
	EasyTcpServer server;
	//server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);

	while (true)
	{
		server.OnRun();
	}

	server.Close();
	getchar();
	return 0;
}