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
$$HISTORY$$
====================================================================================================*/

#ifndef _EASY_TCP_SERVER_INCLUDED
#define _EASY_TCP_SERVER_INCLUDED

#ifdef _WIN32
	#define FD_SETSIZE      2506
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
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

#ifndef RECVBUFSIZE
#define RECVBUFSIZE 10240
#endif

#define THREADS_COUNT 4

namespace
{
	// 客户端对象，包含缓冲区
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

		inline void SendData(const DataHeader* header)
		{
			send(_sockFd, (const char*)header, header->dataLength, 0);
		}

	private:
		SOCKET _sockFd = INVALID_SOCKET; // client socket
		size_t _lastPos = 0;
		char _msgBuf[RECVBUFSIZE * 10] = { 0 };
	};

	class INetEvent
	{
	public:
		virtual void OnNetJoin(Client*) = 0;

		virtual void OnNetLeave(Client*) = 0;

		virtual void OnNetMsg(Client*, DataHeader*) = 0;

		virtual void OnNetRecv(Client*) = 0;
	};

	class CellServer
	{
	public:
		CellServer(SOCKET sock, INetEvent* event) : _sock(sock), _event(event)
		{
			FD_ZERO(&_readFds);
		}

		~CellServer()
		{
			if (_sock != INVALID_SOCKET && !_clients.empty())
			{
				for (auto& client : _clients)
				{
					delete client.second;
					_sock = INVALID_SOCKET;
				}
			}
		}

		void OnRun()
		{
			fd_set allset;
			while (true)
			{
				if (!IsRunning())
					return;

				// 从_clientsBuf中取出所有可用的client
				if (!_clientsBuf.empty())
				{
					std::lock_guard<std::mutex> lock(_mutex);
					for (auto client : _clientsBuf)
					{
						_maxFd = _maxFd > client->GetSockFd() ? _maxFd : client->GetSockFd();
						_clients[client->GetSockFd()] = client;
						FD_SET(client->GetSockFd(), &_readFds);
					}

					//std::cout << "Thread: " << std::this_thread::get_id() << " added " << _clientsBuf.size() << " clients\n";
					_clientsBuf.clear();
				}

				if (_clients.empty())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}

				// 将allset传入select，而_readFds只保存需要select的值，循环一遍后重新将其赋给allset
				//fd_set allset = _readFds;
				//此处使用memcpy fd_set赋值效率
				memcpy(&allset, &_readFds, sizeof(fd_set));

				//timeval t{ 0, 20 };
				int ret = select(_maxFd + 1, &allset, nullptr, nullptr, nullptr);

				if (ret < 0)
				{
					printf("Thread: select error\n");
					Close();
					return;
				}
				else if (ret == 0)
					continue;

#ifdef _WIN32
				std::map<SOCKET, Client*>::iterator iter;
				for (size_t i = 0; i < allset.fd_count; i++)
				{
					if ((iter = _clients.find(allset.fd_array[i])) != _clients.end() && RecvData(iter->second) == -1)
					{
						FD_CLR(allset.fd_array[i], &_readFds);
						delete iter->second;
						_event->OnNetLeave(iter->second);
						_clients.erase(iter);
					}
				}
#else
				// Linux的select与Windows实现不同，windows在fd_set的member-fd_array里存放有所有的socket，不用遍历所有clients判断是否FD_ISSET
				// Linux上需要遍历所有clients
				for (auto iter = _clients.begin(); iter != _clients.end();)
				{
					if (FD_ISSET(iter->first, &allset) && (RecvData(iter->second)) == -1)
					{
						FD_CLR(iter->first, &allset);
						delete iter->second;
						_event->OnNetLeave(iter->second);
						_clients.erase(iter++);
			}
					else
						iter++;
				}
#endif 
			}
             
		}

		inline bool IsRunning()
		{
			return (_sock != INVALID_SOCKET);
		}

		void Start()
		{
			// std::mem_fun, 参数可以是对象指针或对象
			_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
			_thread.detach();
		}

		inline size_t GetClientsNum()
		{
			return _clients.size() + _clientsBuf.size();
		}

		int RecvData(Client* client)
		{
			int cmdLen;

			_event->OnNetRecv(client);
			if ((cmdLen = recv(client->GetSockFd(), _recvBuf, RECVBUFSIZE, 0)) <= 0)
			{
				//printf("Server<%d>: client<%d> has disconnected...\n", (int)_sock, (int)client->GetSockFd());
				_event->OnNetLeave(client);
				return -1;
			}

			memcpy(client->GetMsgBuf() + client->GetLastPos(), _recvBuf, (size_t)cmdLen);
			client->SetLastPos(client->GetLastPos() + cmdLen);

			// 当消息缓冲区的数据长度大于一个Dataheader的长度， 而且大于消息长度的时候
			// &&的短路运算，第一个条件成立时才判断第二个条件
			size_t msgLength;
			while (client->GetLastPos() >= sizeof(DataHeader) && (client->GetLastPos() >= (msgLength = ((DataHeader*)client->GetMsgBuf())->dataLength)))
			{
				OnNetMsg(client, (DataHeader*)client->GetMsgBuf());
				memcpy(client->GetMsgBuf(), client->GetMsgBuf() + msgLength, client->GetLastPos() - msgLength);
				client->SetLastPos(client->GetLastPos() - msgLength);
			}

			return 0;
		}

		void OnNetMsg(Client* client, DataHeader* header)
		{
			_event->OnNetMsg(client, header);
		}

		void Close()
		{
			if (_sock == INVALID_SOCKET)
				return;

			// Close sockets
			for (auto sock : _clients)
				delete sock.second;

			printf("Server<%d> Going to close..\n", (int)_sock);
		}

		void AddClient(Client* client)
		{
			// 实际应用此处可以对client进行验证，是否为非法链接
			std::lock_guard<std::mutex> lg(_mutex);
			_clientsBuf.push_back(client);
		}

	private:
		SOCKET _sock = INVALID_SOCKET;
		INetEvent* _event = nullptr;
		SOCKET _maxFd = INVALID_SOCKET;
		fd_set _readFds;
		char _recvBuf[RECVBUFSIZE];
		std::mutex _mutex;

		// 因为Client类较大，为防止直接存放Client对象导致栈空间不够，存放指针，每次Client的对象用new在堆空间分配
		// 所以这里使用Client*
		std::vector<Client*> _clientsBuf;

		// 24-Nov-2020  由于FD_SET处经常根据socket查询Client*, 此处保存clients的容器由vector改为map，便于查询
		std::map<SOCKET, Client*> _clients;
		std::thread _thread;
	};
}

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
		auto newClient = new Client;

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
			OnNetJoin(client);
		}
	}

	void Close()
	{
		if (_sock == INVALID_SOCKET)
			return;

		// Close sockets
		for (auto cell : _cellServers)
			delete cell;

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
			auto cellServer = new CellServer(_sock, this);
			_cellServers.push_back(cellServer);
			cellServer->Start();
		}
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

	virtual void OnNetMsg(Client* client, DataHeader* header)
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

	bool IsRunning()
	{
		return (_sock != INVALID_SOCKET);
	}

protected:
	std::atomic_int _recvCount = 0;

private:
	SOCKET _sock = INVALID_SOCKET;
	std::atomic_int _msgCount = 0;
	std::atomic_int _clientsCount = 0;

	CELLTimestamp _cellTimer;
	std::vector<CellServer*> _cellServers;
};

#endif