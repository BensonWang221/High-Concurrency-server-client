/*==================================================================================================
   Date                        Description of Change
14-Nov-2020           1. ①First version
						 ②为了方便，暂时不把声明和实现分开实现了
16-Nov-2020           1. 解决粘包、少包问题：通过二级缓冲区，循环处理缓冲区数据
                      2. 每个Client对应一个缓冲区
17-Nov-2020           1. 改进server receive data
18-Nov-2020           1. 添加std高精度计时器测试数据处理能力
19-Nov-2020           1. 修复用max_element获得maxFd的bug
21-Nov-2020           1. 改进select的_readFds, 不再每次都重新FDSET, 增加一个备份来传入select

$$HISTORY$$
====================================================================================================*/

#ifndef _EASY_TCP_SERVER_INCLUDED
#define _EASY_TCP_SERVER_INCLUDED

#ifdef _WIN32
	#define FD_SETSIZE      10010
	#define WIN32_LEAN_AND_MEAN
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <Windows.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
	#define strcpy_s strcpy
	#define closksocket close
#endif
#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

#ifndef RECVBUFSIZE
#define RECVBUFSIZE 10240
#endif

namespace
{
	class Client
	{
	public:
		virtual ~Client()
		{
			if (_sockFd != INVALID_SOCKET)
				closesocket(_sockFd);
		}

		inline SOCKET GetSockFd() const
		{
			return _sockFd;
		}

		inline char* GetMsgBuf()
		{
			return _msgBuf;
		}

		inline SOCKET SetSockFd(const SOCKET sock)
		{
			return (_sockFd = sock);
		}

		inline size_t GetLastPos() const
		{
			return _lastPos;
		}

		inline void SetLastPos(const size_t pos)
		{
			_lastPos = pos;
		}

	private:
		SOCKET _sockFd = INVALID_SOCKET; // client socket
		size_t _lastPos = 0;
		char _msgBuf[RECVBUFSIZE * 10] = { 0 };
	};
}

class EasyTcpServer
{
public:
	EasyTcpServer()
	{
		FD_ZERO(&_readFds);
	}

	virtual ~EasyTcpServer()
	{
		Close();
	}

	void InitSocket()
	{
		if (_sock != INVALID_SOCKET)
		{
			printf("Close the previous initialized socket<%d>..\n", (int)_sock);
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

		_maxFd = _sock;
		printf("Create client socket<%d> successfully..\n", (int)_sock);
	}

	int Bind(const char* ip, const unsigned short port)
	{
		if (_sock == INVALID_SOCKET)
			InitSocket();

		sockaddr_in _sin;
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);

#ifdef _WIN32
		if (ip == nullptr)
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		else
			inet_pton(AF_INET, ip, (void*)&_sin.sin_addr.S_un.S_addr);
#else
		if (ip == nullptr)
			_sin.sin_addr.s_addr = INADDR_ANY;
		else
			inet_pton(AF_INET, ip, (void*)&_sin.sin_addr.s_addr);
#endif
		if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR)
		{
			printf("server<%d> bind port<%d> error...\n", (int)_sock, port);
			return -1;
		}

		printf("server<%d> bind port<%d> success...\n", (int)_sock, port);

		return 0;
	}

	int Listen(const int n)
	{
		if (n <= 0 || _sock == INVALID_SOCKET)
		{
			printf("server listen error\n");
			return -1;
		}

		if (listen(_sock, n) == SOCKET_ERROR)
		{
			printf("server<%d> listen error\n", (int)_sock);
			return -1;
		}

		FD_SET(_sock, &_readFds);
		printf("server<%d> listen success\n", (int)_sock);
		return 0;
	}

	SOCKET Accept()
	{
		sockaddr_in _clientAddr = {};
		socklen_t _clientAddrLen = sizeof(_clientAddr);
		auto newClient = new Client;

		char clientAddr[1024];
		if ((newClient->SetSockFd(accept(_sock, (sockaddr*)&_clientAddr, &_clientAddrLen))) == INVALID_SOCKET)
		{
			printf("server<%d> accept an invalid client socket\n", (int)_sock);
			return INVALID_SOCKET;
		}

		_maxFd = _maxFd > newClient->GetSockFd() ? _maxFd : newClient->GetSockFd();
		NewUserJoin newUserJoin;
		newUserJoin.sock = newClient->GetSockFd();
		SendDataToAll((const DataHeader*)&newUserJoin);

		FD_SET(newClient->GetSockFd(), &_readFds);
		_clients.push_back(newClient);

		printf("Server<%d>: New client<%d> login, IP: %s, Number.%d\n", (int)_sock, (int)newClient->GetSockFd(), inet_ntop(AF_INET, (void*)&_clientAddr.sin_addr, clientAddr, sizeof(clientAddr)), _clients.size());
		return newClient->GetSockFd();
	}

	void Close()
	{
		if (_sock == INVALID_SOCKET)
			return;

		// Close sockets
		for (auto sock : _clients)
			delete sock;

		closesocket(_sock);

#ifdef _WIN32
		WSACleanup();
#endif
		printf("Server<%d> Going to close..\n", (int)_sock);
	}

	void OnRun()
	{
		// 将allset传入select，而_readFds只保存需要select的值，循环一遍后重新将其赋给allset
		fd_set allset = _readFds;

		//timeval t{ 0, 0 };
		int ret = select(_maxFd + 1, &allset, nullptr, nullptr, nullptr);

		if (ret < 0)
		{
			printf("select error\n");
			Close();
			return;
		}

		if (FD_ISSET(_sock, &allset))
		{
			FD_CLR(_sock, &allset);
			Accept();
		}

/*#ifdef _WIN32
		for (size_t i = 0; i < _readFds.fd_count; i++)
		{
			SOCKET sock = _readFds.fd_array[i];
			if (RecvData(sock) == -1)
			{
				auto iter = std::find_if(_clients.begin(), _clients.end(), [sock](Client* client)
					{
						return client->GetSockFd() == sock;
					});

				if (iter != _clients.end())
				{
					delete (*iter);
					_clients.erase(iter);
				}
			}
		}
#else*/
		// For Linux, fd_set is different, so need to go through all clients
		// 为了不在每次clientSock退出时再find遍历一遍，做一下改进
		// 注意！！！！此处要注意vector在erase时的迭代器陷阱！！！
		SOCKET sock;
		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			// 16-Nov-2020  将两个连着的if条件放在一起
			if (FD_ISSET((sock = (*iter)->GetSockFd()), &allset) && RecvData(*iter) == -1)
			{
				FD_CLR(sock, &_readFds);
				delete (*iter);
				iter = _clients.erase(iter); // vector在erase以后元素发生移动，后续迭代器失效，erase返回元素移动后有效的下一个元素迭代器，需要重新赋值给iter！！！！！！！！！！
#ifndef _WIN32
				// Windows上无需寻找maxFd

				// 修复此处bug，有两处
				// 1. 在用max_element时lambda函数有两个参数，因此需要保证_clients.size() > 1，否则会有错误
				// 2. 之前if条件没有!empty(), 会引起段错误, 因为在只有最后一个元素且被erase后，_clients[0]会引起segment fault
				// 其实这里手动遍历_clients找到最大值就可以了，先不改了作为错误示范
				if (sock == _maxFd && !_clients.empty())
				{
					_maxFd = _clients.size() > 1 ? (*(std::max_element(_clients.begin(), _clients.end(), [](Client* client1, Client* client2)
						{
							return client1->GetSockFd() < client2->GetSockFd();
						})))->GetSockFd() : _clients[0]->GetSockFd();

						// 此时需要判断client socket最大值和server socket谁最大
						_maxFd = _maxFd < _sock ? _sock : _maxFd;
			}
#endif
				continue;
		}
			++iter;
	}

//#endif              
	}

	int RecvData(Client* client)
	{
		_recvCount++;
		auto tSection = _cellTimer.GetElapsedTimeInSecond();
		// 每间隔1.0 ms 打印接收数量
		if (tSection >= 1.0)
		{
			printf("server<%d>: tSection: %lf\trecvCount: %u\n", _sock, tSection, _recvCount);
			_recvCount = 0;
			_cellTimer.Update();
		}

		int cmdLen;

		if ((cmdLen = recv(client->GetSockFd(), _recvBuf, RECVBUFSIZE, 0)) <= 0)
		{
			printf("Server<%d>: client<%d> has disconnected...\n", (int)_sock, (int)client->GetSockFd());
			return -1;
		}

		memcpy(client->GetMsgBuf() + client->GetLastPos(), _recvBuf, (size_t)cmdLen);
		client->SetLastPos(client->GetLastPos() + cmdLen);

		// 当消息缓冲区的数据长度大于一个Dataheader的长度， 而且大于消息长度的时候
		// &&的短路运算，第一个条件成立时才判断第二个条件
		size_t msgLength;
		while (client->GetLastPos() >= sizeof(DataHeader) && (client->GetLastPos() >= (msgLength = ((DataHeader*)client->GetMsgBuf())->dataLength)))
		{
			OnNetMsg(client->GetSockFd(), client->GetMsgBuf());
			memcpy(client->GetMsgBuf(), client->GetMsgBuf() + msgLength, client->GetLastPos() - msgLength);
			client->SetLastPos(client->GetLastPos() - msgLength);
		}

		return 0;
	}

	virtual void OnNetMsg(SOCKET clientSock, char* header)
	{
		switch (((DataHeader*)header)->cmd)
		{
		case CMD_LOGIN:
		{
			// �ж��û�������
			//printf("User: %s, password: %s has logged in\n", ((Login*)header)->userName, ((Login*)header)->password);

			LoginResult loginResult;
			loginResult.result = 1;
			SendData(clientSock, (const DataHeader*)&loginResult);
		}
		break;

		case CMD_LOGOUT:
		{
			//printf("User<%d>: %s has logged out\n", clientSock, ((Logout*)header)->userName);

			LogoutResult logoutResult;
			logoutResult.result = 1;
			SendData(clientSock, (const DataHeader*)&logoutResult);
		}
		break;

		default:
		{
			/*DataHeader errheader;
			errheader.cmd = CMD_ERROR;
			errheader.dataLength = sizeof(DataHeader);
			SendData(clientSock, (const DataHeader*)&errheader);*/
			printf("Server<%d> received undefined message, datalength = %d..\n", _sock, ((DataHeader*)header)->dataLength);
		}
		}
	}

	int SendData(SOCKET clientSock, const DataHeader* header)
	{
		if (!IsRunning() || clientSock == INVALID_SOCKET || !header)
		{
			printf("Server<%d> send data to client<%d> error...\n", (int)_sock, (int)clientSock);
			return -1;
		}

		return (send(clientSock, (const char*)header, header->dataLength, 0));
	}

	inline void SendDataToAll(const DataHeader* header)
	{
		for (auto sock : _clients)
			SendData(sock->GetSockFd(), header);
	}

	bool IsRunning()
	{
		return (_sock != INVALID_SOCKET);
	}

private:
	SOCKET _sock = INVALID_SOCKET;
	SOCKET _maxFd = INVALID_SOCKET;
	char _recvBuf[RECVBUFSIZE];
	fd_set _readFds;

	// 因为Client类较大，为防止直接存放Client对象导致栈空间不够，存放指针，每次Client的对象用new在堆空间分配
	// 所以这里使用Client*
	std::vector<Client*> _clients;
	CELLTimestamp _cellTimer;
	size_t _recvCount = 0;
};

#endif