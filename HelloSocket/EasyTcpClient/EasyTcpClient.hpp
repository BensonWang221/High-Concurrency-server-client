/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           1. ①First version
                         ②为了方便，暂时不把声明和实现分开实现了
15-Nov-2020           1. 测试粘包问题
16-Nov-2020           1. 解决粘包、少包问题：通过二级缓冲区，循环处理缓冲区数据

$$HISTORY$$
====================================================================================================*/

#ifndef _EASY_TCP_CLIENT_INCLUDED
#define _EASY_TCP_CLIENT_INCLUDED

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
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
	#define strcpy_s strcpy
#endif
#include "MessageHeader.hpp"
#include <stdlib.h>
#include <iostream>

#ifndef RECVBUFSIZE
#define RECVBUFSIZE 10240
#endif

class EasyTcpClient
{
public:
	EasyTcpClient()
	{

	}
	virtual ~EasyTcpClient()
	{

	}

	// 初始化
	void InitSocket()
	{
		if (_sock != INVALID_SOCKET)
		{
			printf("Close the previous initialized socket<%d>..\n", _sock);
			Close();
		}

		// 启动WinSock2服务
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);

#endif
		_sock = socket(AF_INET, SOCK_STREAM, 0);

		if (_sock == INVALID_SOCKET)
			printf("create client socket failed...\n");

		printf("Create client socket<%d> successfully..\n", _sock);
	}

	int  Connect(const char* ip, const unsigned short port)
	{
		if (_sock == INVALID_SOCKET)
			InitSocket();
		sockaddr_in _serverAddr = {};
		_serverAddr.sin_family = AF_INET;
		_serverAddr.sin_port = htons(port);
#ifdef _WIN32
		inet_pton(AF_INET, ip, (void*)&_serverAddr.sin_addr.S_un.S_addr);
#else
		inet_pton(AF_INET, ip, (void*)&_serverAddr.sin_addr.s_addr);
#endif

		int ret = -1;
		if ((ret = connect(_sock, (sockaddr*)&_serverAddr, sizeof(_serverAddr))) == SOCKET_ERROR)
		{
			printf("connect error\n");
			return ret;
		}

		else
			printf("Connect to server success\n");

		return ret;
	}

	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			WSACleanup();
			closesocket(_sock);
#else 
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}

	bool OnRun()
	{
		if (!IsRunning())
		{
			printf("socket<%d> is not running\n", _sock);
			return false; 
		}

		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(_sock, &readFds);

		timeval t{ 0, 0 };
		if (select(_sock + 1, &readFds, nullptr, nullptr, &t) < 0)
		{
			printf("Client<%d> select error\n", _sock);
			Close();
			return false;
		}

		if (FD_ISSET(_sock, &readFds))
		{
			FD_CLR(_sock, &readFds);
			if (RecvData() == -1)
				return false;
		}
		return true;
	}
	
	int RecvData()
	{
		
		int cmdLen;

		/*if ((cmdLen = recv(_sock, _recvBuf, sizeof(DataHeader), 0)) <= 0)
		{
			printf("Disconnect with server, going to close\n");
			return -1;
		}

		recv(_sock, _recvBuf + sizeof(DataHeader), ((DataHeader*)_recvBuf)->dataLength - sizeof(DataHeader), 0);
		OnNetMsg((DataHeader*)_recvBuf);*/

		if ((cmdLen = recv(_sock, _recvBuf, RECVBUFSIZE, 0)) <= 0)
		{
			printf("Disconnect with server, going to close\n");
			return -1;
		}

		memcpy(_msgBuf + _lastPos, _recvBuf, (size_t)cmdLen);
		_lastPos += cmdLen;

		// 当消息缓冲区的数据长度大于一个Dataheader的长度， 而且大于消息长度的时候
		// &&的短路运算，第一个条件成立时才判断第二个条件
		size_t msgLength;
		while (_lastPos >= sizeof(DataHeader) && (_lastPos >= (msgLength = ((DataHeader*)_msgBuf)->dataLength)))
		{
			OnNetMsg((DataHeader*)_msgBuf);
			memcpy(_msgBuf, _msgBuf + msgLength, _lastPos - msgLength);
			_lastPos -= msgLength;
		}
		
		return 0;
	}

	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			//printf("Reveived login result from server: %d\n", ((LoginResult*)header)->result);
		}
		break;

		case CMD_LOGOUT:
		{
			//printf("Reveived login result from server: %d\n", ((LogoutResult*)header)->result);
		}
		break;

		case CMD_NEW_USER_JOIN:
		{
			//printf("A new user has joined: %d, welcome!\n", ((NewUserJoin*)header)->sock);
		}
		break;

		default:
		{
			DataHeader errHeader;
			errHeader.cmd = CMD_ERROR;
			errHeader.dataLength = 0;
			send(_sock, (char*)&errHeader, sizeof(DataHeader), 0);
			printf("client<%d>: message error...\n", _sock);
		}
		}
	}

	inline int SendData(DataHeader* header)
	{
		if (IsRunning() && header)
			return send(_sock, (const char*)header, header->dataLength, 0);

		return SOCKET_ERROR;
	}

	inline bool IsRunning()
	{
		return (_sock != INVALID_SOCKET);
	}


protected:
	
private:
	SOCKET _sock = INVALID_SOCKET;

	// 接收缓冲区
	char _recvBuf[RECVBUFSIZE];

	// 消息缓冲区 - 第二缓冲区
	char _msgBuf[RECVBUFSIZE * 10];

	// 上一次消息的头部位置
	size_t _lastPos = 0;
};

#endif
