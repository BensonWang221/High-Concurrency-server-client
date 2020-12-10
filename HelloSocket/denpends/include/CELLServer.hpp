/*==================================================================================================
   Date                        Description of Change
06-Dec-2020           1. ①First version, 将各个类分开实现，优化代码结构
						 ②为每个客户端添加心跳检测heart check
07-Dec-2020           1. 使用封装好的CELLThread来替代原有线程启动以及线程函数
$$HISTORY$$
====================================================================================================*/

#ifndef _CELL_SERVER_INCLUDED
#define _CELL_SERVER_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Cell.h"
#include "CELLClient.hpp"
#include "INetEvent.hpp"
#include "CELLTask.hpp"

class CellServer
{
public:
	CellServer(int id, INetEvent* event) : _id(id), _event(event)
	{
		FD_ZERO(&_readFds);
	}

	void OnRun(CELLThread* thread)
	{
		fd_set allset;
		while (thread->IsRunning())
		{
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
				_heartCheckTime = CELLTime::GetCurrentTimeInMilliSec();
				continue;
			}

			// 将allset传入select，而_readFds只保存需要select的值，循环一遍后重新将其赋给allset
			//fd_set allset = _readFds;
			//此处使用memcpy fd_set赋值效率
			memcpy(&allset, &_readFds, sizeof(fd_set));

			timeval t{ 0, 1 };
			int ret = select(_maxFd + 1, &allset, nullptr, nullptr, &t);

			if (ret < 0)
			{
				printf("CELLServer: select error\n");

				thread->Exit();
				// <07-Dec-2020>
				// 注意现在OnRun和Close现在在同一个线程中，且Close需要OnRun结束后唤醒
				// 在同一个线程中使用条件变量在执行中间调用wait后阻塞等待会导致死锁！！！
				/*Close();
				return;*/
			}
			//else if (ret == 0)
			//	continue;
			ReadData(allset);

			// Heart check for every client
			CheckHeartAndSendForClient();
		}
	}

	void ReadData(fd_set& allset)
	{

#ifdef _WIN32
		std::map<SOCKET, Client*>::iterator iter;
		for (size_t i = 0; i < allset.fd_count; i++)
		{
			if ((iter = _clients.find(allset.fd_array[i])) != _clients.end() && RecvData(iter->second) == -1)
			{
				EraseClient(iter->second);
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
				EraseClient(iter->second);
				_clients.erase(iter++);
			}
			else
				iter++;
		}
#endif 
	}

	void CheckHeartAndSendForClient()
	{
		auto currentTime = CELLTime::GetCurrentTimeInMilliSec();
		auto dt = currentTime - _heartCheckTime;
		for (auto iter = _clients.begin(); iter != _clients.end();)
		{
			if (iter->second->HeartCheck(dt))
			{
				EraseClient(iter->second);
				_clients.erase(iter++);
			}

			else
			{
				iter->second->CheckSendTime(dt);
				iter++;
			}
		}
		_heartCheckTime = currentTime;
	}

	void EraseClient(Client* client)
	{
		_event->OnNetLeave(client);
		FD_CLR(client->GetSockFd(), &_readFds);
		delete client;
	}

	void Start()
	{
		_taskServer.Start();

		// std::mem_fun, 参数可以是对象指针或对象
		_thread.Start(nullptr, [this](CELLThread* thread)
			{
				this->OnRun(thread);
			}, [this](CELLThread* thread)
			{
				this->ClearClients();
			});
	}

	void ClearClients()
	{
		if (!_clients.empty())
		{
			for (auto& client : _clients)
				delete client.second;

			_clients.clear();
		}

		if (!_clientsBuf.empty())
		{
			for (auto client : _clientsBuf)
				delete client;

			_clientsBuf.clear();
		}
	}

	size_t GetClientsNum()
	{
		return _clients.size() + _clientsBuf.size();
	}

	int RecvData(Client* client)
	{
		int cmdLen;

		_event->OnNetRecv(client);
		if ((cmdLen = recv(client->GetSockFd(), client->GetMsgBuf() + client->GetLastRecvPos(), RECVBUFSIZE - client->GetLastRecvPos(), 0)) <= 0)
		{
			//printf("Server<%d>: client<%d> has disconnected...\n", (int)_sock, (int)client->GetSockFd());
			_event->OnNetLeave(client);
			return -1;
		}

		//memcpy(client->GetMsgBuf() + client->GetLastRecvPos(), _recvBuf, (size_t)cmdLen);
		client->SetLastRecvPos(client->GetLastRecvPos() + cmdLen);

		// 当消息缓冲区的数据长度大于一个Dataheader的长度， 而且大于消息长度的时候
		// &&的短路运算，第一个条件成立时才判断第二个条件
		size_t msgLength;
		while (client->GetLastRecvPos() >= sizeof(DataHeader) && (client->GetLastRecvPos() >= (msgLength = ((DataHeader*)client->GetMsgBuf())->dataLength)))
		{
			OnNetMsg(client, (DataHeader*)client->GetMsgBuf());
			memcpy(client->GetMsgBuf(), client->GetMsgBuf() + msgLength, client->GetLastRecvPos() - msgLength);
			client->SetLastRecvPos(client->GetLastRecvPos() - msgLength);
		}

		return 0;
	}

	void OnNetMsg(Client* client, DataHeader* header)
	{
		_event->OnNetMsg(this, client, header);
	}

	void Close()
	{
		_taskServer.Close();
		_thread.Close();

		printf("CellServer Going to close..\n");
	}

	void AddClient(Client* client)
	{
		// 实际应用此处可以对client进行验证，是否为非法链接
		std::lock_guard<std::mutex> lg(_mutex);
		_clientsBuf.push_back(client);
		_condVar.notify_one();
	}

	//void AddSendTask(Client* client, DataHeader* header)
	//{
	//	// 使用lambda表达式来代替原有new一个send task，更简洁，效率更高
	//	_taskServer.AddTask([client, header]()
	//		{
	//			client->SendData(header);
	//			delete header;
	//		});
	//}

private:
	fd_set _readFds;
	CELLThread _thread;

	// 24-Nov-2020  由于FD_SET处经常根据socket查询Client*, 此处保存clients的容器由vector改为map，便于查询
	std::map<SOCKET, Client*> _clients;

	// 因为Client类较大，为防止直接存放Client对象导致栈空间不够，存放指针，每次Client的对象用new在堆空间分配
	// 所以这里使用Client*
	std::vector<Client*> _clientsBuf;

	std::mutex _mutex;
	std::condition_variable _condVar;
	CellTaskServer _taskServer;
	INetEvent* _event = nullptr;
	SOCKET _maxFd = 0;
	time_t _heartCheckTime = CELLTime::GetCurrentTimeInMilliSec();
	int _id = -1;
};

#endif