/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           First version
09-Nov-2020           ①针对Linux在遍历返回的fd_set时，不再每次遍历获取maxFd，而是保存maxFd，在新加或删除时更新maxFd
                      ②注意vector/deque在用iterator遍历时，不能随便erase，注意迭代器陷阱！erase后续iterator失效！！
14-Nov-2020           封装类EasyTcpServer，重写server.cpp
24-Nov-2020           实现MyServer继承EasyTcpServer，扩展功能

$$HISTORY$$
====================================================================================================*/

#include "EasyTcpServer.hpp"

class MyServer : public EasyTcpServer
{
public:
	virtual void OnNetJoin(Client* client)
	{
		EasyTcpServer::OnNetJoin(client);
	}

	virtual void OnNetLeave(Client* client)
	{
		EasyTcpServer::OnNetLeave(client);
	}

	virtual void OnNetMsg(CellServer* cellServer, Client* client, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(cellServer, client, header);
		switch ((header)->cmd)
		{
		case CMD_LOGIN:
		{
			// �ж��û�������
			//printf("User: %s, password: %s has logged in\n", ((Login*)header)->userName, ((Login*)header)->password);

			LoginResult loginResult;
			loginResult.result = 1;
			client->SendData(&loginResult);
			//auto result = new LoginResult;
			//result->result = 1;
			//cellServer->AddSendTask(client, static_cast<DataHeader*>(result));
		}
		break;

		case CMD_LOGOUT:
		{
			//printf("User<%d>: %s has logged out\n", clientSock, ((Logout*)header)->userName);

			LogoutResult logoutResult;
			logoutResult.result = 1;
			//client->SendData((const DataHeader*)&logoutResult);
		}
		break;

		default:
		{
			/*DataHeader errheader;
			errheader.cmd = CMD_ERROR;
			errheader.dataLength = sizeof(DataHeader);
			client->SendData((const DataHeader*)&errheader);*/
			printf("Server<%d> received undefined message, datalength = %d..\n", _sock, ((DataHeader*)header)->dataLength);
		}
		}
	}

	virtual void OnNetRecv(Client* client)
	{
		EasyTcpServer::OnNetRecv(client);
	}
};

int main()
{
	MyServer server;
	//server.InitSocket();
	server.Bind("192.168.0.109", 4567);
	server.Listen(3000);
	server.Start(4);

	while (true)
	{
		server.OnRun();
	}

	server.Close();
	getchar();
	return 0;
}