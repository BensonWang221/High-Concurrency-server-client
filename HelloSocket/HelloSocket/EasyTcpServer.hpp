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
23-Nov-2020           1. 多线程分组处理客户端数据，使用生产者-消费者模型, 主线程accept，将客户端数据分给四个子线程处理
24-Nov-2020           1. 添加INetEvent作为EasyTcpServer的纯虚基类
26-Nov-2020           1. 更改存放clients的容器为map，提高查询和删除效率，因为每次的select返回都要从clients去查找，目前性能瓶颈为select
28-Nov-2020           1. send函数占用时间较多，为减少send函数调用次数，实现定量发送，添加发送缓冲区
02-Dec-2020           1. 引入内存池和标准库智能指针shared_ptr

$$HISTORY$$
====================================================================================================*/

#ifndef _EASY_TCP_SERVER_INCLUDED
#define _EASY_TCP_SERVER_INCLUDED

#include "Cell.h"
#include <algorithm>
#include <stdlib.h>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
#include "CELLClient.hpp"
#include "INetEvent.hpp"
#include "CELLServer.hpp"

#define THREADS_COUNT 4


class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer()
	{
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
		
		printf("server<%d> listen success\n", (int)_sock);
		return 0;
	}

	SOCKET Accept()
	{
		sockaddr_in _clientAddr = {};
		socklen_t _clientAddrLen = sizeof(_clientAddr);
		//auto newClient = std::make_shared<Client>();
		Client* newClient(new Client());

		//char clientAddr[1024];
		if ((newClient->SetSockFd(accept(_sock, (sockaddr*)&_clientAddr, &_clientAddrLen))) == INVALID_SOCKET)
		{
			printf("server<%d> accept an invalid client socket\n", (int)_sock);
			return INVALID_SOCKET;
		}
		AddClientToCell(newClient);
		OnNetJoin(newClient);
		//printf("Server<%d>: New client join, ip = %s, port = %d\n", _sock, inet_ntop(AF_INET, (const void*)&_clientAddr.sin_addr.S_un, clientAddr, sizeof(clientAddr)), ntohs(_clientAddr.sin_port));
	}

	void AddClientToCell(Client* client)
	{
		if (!_cellServers.empty())
		{
			auto minCellServer = _cellServers[0];
			for (auto cellServer : _cellServers)
				minCellServer = minCellServer->GetClientsNum() < cellServer->GetClientsNum() ? minCellServer : cellServer;

			minCellServer->AddClient(client);
		}
	}

	void Close()
	{
		if (_sock == INVALID_SOCKET)
			return;

		closesocket(_sock);

#ifdef _WIN32
		WSACleanup();
#endif
		printf("Server<%d> Going to close..\n", (int)_sock);
	}

	void Start(int threadCount)
	{
		if (_sock == INVALID_SOCKET)
			return;

		for (int i = 0; i < threadCount; i++)
		{
			//auto cellServer = new CellServer(_sock, this);
			_cellServers.push_back(new CellServer(_sock, this));
		}

		for (auto& cellServer : _cellServers)
			cellServer->Start();
	}

	void OnRun()
	{
		MsgPerSecond();

		fd_set readFd;
		FD_ZERO(&readFd);
		FD_SET(_sock, &readFd);
		timeval t{ 0, 0 };
		int ret = select(_sock + 1, &readFd, nullptr, nullptr, &t);

		if (ret < 0)
		{
			printf("select error\n");
			Close();
			return;
		}

		if (FD_ISSET(_sock, &readFd))
		{
			FD_CLR(_sock, &readFd);
			Accept();
		}
//#endif              
	}

	void MsgPerSecond()
	{
		auto tSection = _cellTimer.GetElapsedTimeInSecond();

		//每间隔1.0 ms 打印接收数量
		if (tSection >= 1.0)
		{
			size_t totalClientsNum = 0;
			for (auto cellSer : _cellServers)
				totalClientsNum += cellSer->GetClientsNum();
			printf("server<%d>: tSection: %lf\tclients<%u>recvCount: %d\tmsgCount: %d\n", _sock, tSection, totalClientsNum, _recvCount.load(), _msgCount.load());
			_recvCount = 0;
			_msgCount = 0;
			_cellTimer.Update();
		}

	}

	virtual void OnNetMsg(CellServer* cellServer, Client* client, DataHeader* header)
	{
		_msgCount++;
	}

	virtual void OnNetLeave(Client* client)
	{
		_clientsCount--;
	}

	virtual void OnNetJoin(Client* client)
	{
		_clientsCount++;
	}

	virtual void OnNetRecv(Client* client)
	{
		_recvCount++;
	}

	bool IsRunning()
	{
		return (_sock != INVALID_SOCKET);
	}

protected:
	std::atomic_int _recvCount = 0;
	SOCKET _sock = INVALID_SOCKET;

private:
	std::atomic_int _msgCount = 0;
	std::atomic_int _clientsCount = 0;

	CELLTimestamp _cellTimer;
	std::vector<CellServer*> _cellServers;
};

#endif