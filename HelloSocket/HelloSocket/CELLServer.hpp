/*==================================================================================================
   Date                        Description of Change
06-Dec-2020           1. ��First version, ��������ֿ�ʵ�֣��Ż�����ṹ
                         ��Ϊÿ���ͻ�������������heart check

$$HISTORY$$
====================================================================================================*/

#ifndef _CELL_SERVER_INCLUDED
#define _CELL_SERVER_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include "Cell.h"
#include "CELLClient.hpp"
#include "INetEvent.hpp"
#include "CELLTask.hpp"

class CellServer
{
public:
	CellServer(SOCKET sock, INetEvent* event) : _sock(sock), _event(event)
	{
		FD_ZERO(&_readFds);
	}

	~CellServer()
	{
		//if (_sock != INVALID_SOCKET && !_clients.empty())
		//{
		//	for (auto& client : _clients)
		//	{
		//		delete client.second;
		//		_sock = INVALID_SOCKET;
		//	}
		//}
	}

	void OnRun()
	{
		fd_set allset;
		while (true)
		{
			if (!IsRunning())
				return;

			// ��_clientsBuf��ȡ�����п��õ�client
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

			// ��allset����select����_readFdsֻ������Ҫselect��ֵ��ѭ��һ������½��丳��allset
			//fd_set allset = _readFds;
			//�˴�ʹ��memcpy fd_set��ֵЧ��
			memcpy(&allset, &_readFds, sizeof(fd_set));

			timeval t{ 0, 1 };
			int ret = select(_maxFd + 1, &allset, nullptr, nullptr, &t);

			if (ret < 0)
			{
				printf("Thread: select error\n");
				Close();
				return;
			}
			//else if (ret == 0)
			//	continue;
			ReadData(allset);

			// Heart check for every client
			CheckHeartForClient();
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
		// Linux��select��Windowsʵ�ֲ�ͬ��windows��fd_set��member-fd_array���������е�socket�����ñ�������clients�ж��Ƿ�FD_ISSET
		// Linux����Ҫ��������clients
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

	void CheckHeartForClient()
	{
		auto currentTime = CELLTime::GetCurrentTimeInMilliSec();
		for (auto iter = _clients.begin(); iter != _clients.end();)
		{
			if (iter->second->HeartCheck(currentTime - _heartCheckTime))
			{
				EraseClient(iter->second);
				_clients.erase(iter++);
			}

			else
				iter++;
		}
		_heartCheckTime = currentTime;
	}

	inline void EraseClient(Client* client)
	{
		_event->OnNetLeave(client);
		FD_CLR(client->GetSockFd(), &_readFds);
		delete client;
	}

	inline bool IsRunning()
	{
		return (_sock != INVALID_SOCKET);
	}

	void Start()
	{
		// std::mem_fun, ���������Ƕ���ָ������
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
		if ((cmdLen = recv(client->GetSockFd(), client->GetMsgBuf() + client->GetLastRecvPos(), RECVBUFSIZE - client->GetLastRecvPos(), 0)) <= 0)
		{
			//printf("Server<%d>: client<%d> has disconnected...\n", (int)_sock, (int)client->GetSockFd());
			_event->OnNetLeave(client);
			return -1;
		}

		//memcpy(client->GetMsgBuf() + client->GetLastRecvPos(), _recvBuf, (size_t)cmdLen);
		client->SetLastRecvPos(client->GetLastRecvPos() + cmdLen);

		// ����Ϣ�����������ݳ��ȴ���һ��Dataheader�ĳ��ȣ� ���Ҵ�����Ϣ���ȵ�ʱ��
		// &&�Ķ�·���㣬��һ����������ʱ���жϵڶ�������
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
		if (_sock == INVALID_SOCKET)
			return;

		// Close sockets
		for (auto sock : _clients)
			delete sock.second;

		printf("Server<%d> Going to close..\n", (int)_sock);
	}

	void AddClient(Client* client)
	{
		// ʵ��Ӧ�ô˴����Զ�client������֤���Ƿ�Ϊ�Ƿ�����
		std::lock_guard<std::mutex> lg(_mutex);
		_clientsBuf.push_back(client);
	}

	//void AddSendTask(Client* client, DataHeader* header)
	//{
	//	// ʹ��lambda���ʽ������ԭ��newһ��send task������࣬Ч�ʸ���
	//	_taskServer.AddTask([client, header]()
	//		{
	//			client->SendData(header);
	//			delete header;
	//		});
	//}

private:
	SOCKET _sock = INVALID_SOCKET;
	INetEvent* _event = nullptr;
	SOCKET _maxFd = 0;
	fd_set _readFds;
	std::mutex _mutex;

	time_t _heartCheckTime = CELLTime::GetCurrentTimeInMilliSec();

	// ��ΪClient��ϴ�Ϊ��ֱֹ�Ӵ��Client������ջ�ռ䲻�������ָ�룬ÿ��Client�Ķ�����new�ڶѿռ����
	// ��������ʹ��Client*
	std::vector<Client*> _clientsBuf;

	CellTaskServer _taskServer;

	// 24-Nov-2020  ����FD_SET����������socket��ѯClient*, �˴�����clients��������vector��Ϊmap�����ڲ�ѯ
	std::map<SOCKET, Client*> _clients;
	std::thread _thread;
};

#endif