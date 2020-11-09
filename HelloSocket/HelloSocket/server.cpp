/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           1. First version
09-Nov-2020           2. ①针对Linux在遍历返回的fd_set时，不再每次遍历获取maxFd，而是保存maxFd，在新加或删除时更新maxFd
                         ②注意vector/deque在用iterator遍历时，不能随便erase，注意迭代器陷阱！erase后续iterator失效！！

$$HISTORY$$
====================================================================================================*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
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

#pragma comment(lib, "ws2_32.lib")

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

struct DataHeader
{
	DataHeader() {}
	DataHeader(short length, short c) : dataLength(length), cmd(c) {}

	short dataLength;
	short cmd;
};

struct Login : public DataHeader
{
	Login() : DataHeader(sizeof(Login), CMD_LOGIN) {}

	char userName[32];
	char password[32];
};

struct LoginResult : public DataHeader
{
	LoginResult() : DataHeader(sizeof(LoginResult), CMD_LOGIN_RESULT) {}
	int result;
};

struct Logout : public DataHeader
{
	Logout() : DataHeader(sizeof(Logout), CMD_LOGOUT) {}
	char userName[32];
};

struct LogoutResult : public DataHeader
{
	LogoutResult() : DataHeader(sizeof(LogoutResult), CMD_LOGOUT_RESULT) {}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin() : DataHeader(sizeof(NewUserJoin), CMD_NEW_USER_JOIN) {}
	int sock;
};

std::vector<SOCKET> g_clients;

int handleMsg(SOCKET _clientSock)
{
	char recvBuf[1024];
	int cmdLen;

	if ((cmdLen = recv(_clientSock, recvBuf, sizeof(DataHeader), 0)) <= 0)
	{
		printf("Client<%d> has logged off\n", _clientSock);
		return -1;
	}

	switch (((DataHeader*)recvBuf)->cmd)
	{
	case CMD_LOGIN:
	{
		recv(_clientSock, recvBuf + sizeof(DataHeader), ((DataHeader*)recvBuf)->dataLength - sizeof(DataHeader), 0);
		// �ж��û�������
		printf("User: %s, password: %s has logged in\n", ((Login*)recvBuf)->userName, ((Login*)recvBuf)->password);

		LoginResult loginResult;
		loginResult.result = 1;
		send(_clientSock, (char*)&loginResult, sizeof(LoginResult), 0);
	}
	break;

	case CMD_LOGOUT:
	{
		recv(_clientSock, recvBuf + sizeof(DataHeader), ((DataHeader*)recvBuf)->dataLength - sizeof(DataHeader), 0);
		printf("User<%d>: %s has logged out\n", _clientSock, ((Logout*)recvBuf)->userName);

		LogoutResult logoutResult;
		logoutResult.result = 1;
		send(_clientSock, (char*)&logoutResult, sizeof(LoginResult), 0);
	}
	break;

	default:
	{
		DataHeader header;
		header.cmd = CMD_ERROR;
		header.dataLength = 0;
		send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
	}
	}

	return 0;
}

int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif

	// Create a socket using method socket()
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in _sin;
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif
	// bind port and ip
	if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR)
	{
		printf("bind error\n");
		exit(1);
	}

	printf("server bind port and ip address successfully!\n");

	// listen
	if (listen(_sock, 5) == SOCKET_ERROR)
	{
		printf("Listen error\n");
		exit(1);
	}

	printf("server listen successfully\n");

	SOCKET maxFd = _sock;
	while (true)
	{
		fd_set readFds, writeFds, exceptFds;
		FD_ZERO(&readFds);
		FD_ZERO(&writeFds);
		FD_ZERO(&exceptFds);

		FD_SET(_sock, &readFds);
		FD_SET(_sock, &writeFds);
		FD_SET(_sock, &exceptFds);

		for (auto client : g_clients)
			FD_SET(client, &readFds);

		timeval t{ 0, 0 };

		int ret = select(maxFd + 1, &readFds, &writeFds, &exceptFds, &t);

		if (ret < 0)
		{
			printf("select error\n");
			break;
		}

		if (FD_ISSET(_sock, &readFds))
		{
			FD_CLR(_sock, &readFds);

			// accept
			sockaddr_in _clientAddr = {};
			socklen_t _clientAddrLen = sizeof(_clientAddr);
			SOCKET _clientSock = INVALID_SOCKET;
			char clientAddr[1024];
			if ((_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &_clientAddrLen)) == INVALID_SOCKET)
			{
				printf("accept an invalid client socket\n");
				exit(1);
			}
			maxFd = maxFd > _clientSock ? maxFd : _clientSock;
			NewUserJoin newUserJoin;
			newUserJoin.sock = _clientSock;
			for (auto sock : g_clients)
				send(sock, (const char*)&newUserJoin, sizeof(newUserJoin), 0);

			g_clients.push_back(_clientSock);

			printf("New client<%d> login, IP: %s\n", _clientSock, inet_ntop(AF_INET, (void*)&_clientAddr.sin_addr, clientAddr, sizeof(clientAddr)));
		}

#ifdef _WIN32
		for (size_t i = 0; i < readFds.fd_count; i++)
		{
			SOCKET sock = readFds.fd_array[i];
			if (handleMsg(sock) == -1)
			{
				auto iter = std::find(g_clients.begin(), g_clients.end(), sock);

				if (iter != g_clients.end())
					g_clients.erase(iter);
				closesocket(sock);
			}
		}
#else
		// For Linux, fd_set is different, so need to go through all clients
		// 为了不在每次clientSock退出时再find遍历一遍，做一下改进
		// 注意！！！！此处要注意vector在erase时的迭代器陷阱！！！
		/*for (auto sock : g_clients)
		{
			if (FD_ISSET(sock, &readFds))
			{
				if (handleMsg(sock) == -1)
				{
					auto iter = std::find(g_clients.begin(), g_clients.end(), sock);
					if (iter != g_clients.end())
						g_clients.erase(iter);
					close(sock);
					if (sock == maxFd)
					{
						int maxEle = *std::max_element(g_clients.begin(), g_clients.end());
						maxFd = maxEle > _sock ? maxEle : _sock;
					}
				}
			}
		}*/

		for (auto iter = g_clients.begin(); iter != g_clients.end(); )
		{
			if (FD_ISSET(*iter, &readFds))
			{
				if (handleMsg(*iter) == -1)
				{
					close(*iter);
					iter = g_clients.erase(iter); // vector在erase以后元素发生移动，后续迭代器失效，erase返回元素移动后有效的下一个元素迭代器，需要重新赋值给iter！！！！！！！！！！
					continue;
				}
			}
			++iter;
		}

#endif              
	}

	// Close sockets
	for (auto sock : g_clients)
		closesocket(sock);
#ifdef _WIN32
	closesocket(_sock);
	WSACleanup();
#else
	close(_sock);
#endif
	printf("Going to close..\n");
	getchar();
	return 0;
}