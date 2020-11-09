/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           First version


$$HISTORY$$
====================================================================================================*/




#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define strcpy_s strcpy
#endif
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

char userName[32];

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

int handleMsg(SOCKET _clientSock)
{
	char recvBuf[1024];
	int cmdLen;

	if ((cmdLen = recv(_clientSock, recvBuf, sizeof(DataHeader), 0)) <= 0)
	{
		printf("Disconnect with server, going to close\n");
		return -1;
	}

	switch (((DataHeader*)recvBuf)->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		recv(_clientSock, recvBuf + sizeof(DataHeader), ((DataHeader*)recvBuf)->dataLength - sizeof(DataHeader), 0);
		printf("Reveived login result from server: %d\n", ((LoginResult*)recvBuf)->result);
	}
	break;

	case CMD_LOGOUT:
	{
		recv(_clientSock, recvBuf + sizeof(DataHeader), ((DataHeader*)recvBuf)->dataLength - sizeof(DataHeader), 0);
		printf("Reveived login result from server: %d\n", ((LogoutResult*)recvBuf)->result);
	}
	break;

	case CMD_NEW_USER_JOIN:
	{
		recv(_clientSock, recvBuf + sizeof(DataHeader), ((DataHeader*)recvBuf)->dataLength - sizeof(DataHeader), 0);
		printf("A new user has joined: %d, welcome!\n", ((NewUserJoin*)recvBuf)->sock);
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

void cmdThread(SOCKET sock)
{
	char cmdBuf[128];
	while (true)
	{
		printf("Enter command: \n");
		std::cin.getline(cmdBuf, sizeof(cmdBuf));
		if (strcmp(cmdBuf, "exit") == 0)
		{
			return;
		}

		else if (strcmp(cmdBuf, "login") == 0)
		{
			Login loginInfo;
			std::cout << "Enter user name: \n" << std::endl;
			std::cin.getline(loginInfo.userName, sizeof(loginInfo.userName));
			strcpy_s(userName, loginInfo.userName);
			std::cout << "Enter your password: \n" << std::endl;
			std::cin.getline(loginInfo.password, sizeof(loginInfo.password));
			send(sock, (const char*)&loginInfo, sizeof(loginInfo), 0);
		}

		else if (strcmp(cmdBuf, "logout") == 0)
		{
			Logout logoutInfo;
			strcpy_s(logoutInfo.userName, userName);
			send(sock, (const char*)&logoutInfo, sizeof(Logout), 0);
		}

		else if (strcmp(cmdBuf, "exit") == 0)
		{
			printf("Client is going to close..\n");
#ifdef _WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			break;
		}

		else
		{
			DataHeader header(0, CMD_ERROR);
			send(sock, (const char*)&header, sizeof(DataHeader), 0);
		}
	}
}

int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
#endif

	SOCKET _clientSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_clientSock == INVALID_SOCKET)
	{
		printf("create client socket failed...\n");
		exit(1);
	}

	printf("Create client socket successfully..\n");

	sockaddr_in _serverAddr = {};
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(4567);
#ifdef _WIN32
	inet_pton(AF_INET, "127.0.0.1", (void*)&_serverAddr.sin_addr.S_un.S_addr);
#else
	inet_pton(AF_INET, "127.0.0.1", (void*)&_serverAddr.sin_addr.s_addr);
#endif

	if (connect(_clientSock, (sockaddr*)&_serverAddr, sizeof(_serverAddr)) == SOCKET_ERROR)
	{
		printf("connect error\n");
		exit(1);
	}
	printf("Connect to server success\n");

	std::thread t(cmdThread, _clientSock);

	int ret;
	while (true)
	{
		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(_clientSock, &readFds);

		if ((ret = select(_clientSock + 1, &readFds, nullptr, nullptr, nullptr)) < 0)
		{
			printf("Client select error\n");
			break;
		}

		if (FD_ISSET(_clientSock, &readFds))
		{
			FD_CLR(_clientSock, &readFds);
			if (handleMsg(_clientSock) == -1)
				break;
		}

	}

#ifdef _WIN32
	closesocket(_clientSock);
#else 
	close(_clientSock);
#endif

#ifdef _WIN43
	WSACleanup();
#endif
	printf("Going to close..\n");
	getchar();
	return 0;
}