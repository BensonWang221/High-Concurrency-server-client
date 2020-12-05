/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           First version
09-Nov-2020           根据封装的EasyTcpClient类改进client应用程序
15-Nov-2020           测试粘包问题
21-Nov-2020           四线程创建客户端连接服务端和发送数据

$$HISTORY$$
====================================================================================================*/

#include "EasyTcpClient.hpp"
#include "CELLTimestamp.hpp"
#include <thread>
#include <atomic>

char userName[32];

Login login; 
const int clientsCount = 1000;
const int threadCount = 4;
EasyTcpClient* clients[clientsCount];
std::atomic_int clientNum = 0;
std::atomic_int sendNum = 0;
std::atomic_int readyNum = 0;

void cmdThread(EasyTcpClient* client)
{
	char cmdBuf[128];
	while (true)
	{
		printf("Enter command: \n");
		std::cin.getline(cmdBuf, sizeof(cmdBuf));
		if (strcmp(cmdBuf, "exit") == 0)
		{
			client->Close();
			printf("Return from thread function\n");
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
			client->SendData(&loginInfo);
		}

		else if (strcmp(cmdBuf, "logout") == 0)
		{
			Logout logoutInfo;
			strcpy_s(logoutInfo.userName, userName);
			client->SendData(&logoutInfo);
		}

		else
		{
			DataHeader header(0, CMD_ERROR);
			client->SendData(&header);
		}
	}
}

void RecvThread(size_t begin, size_t end)
{
	while (true)
	{
		for (size_t i = begin; i <= end; i++)
			clients[i]->OnRun();
	}
}

void SendThread(int id)
{
	size_t begin = clientsCount / threadCount * (id - 1);
	size_t end = id != threadCount ? (clientsCount / threadCount * id - 1) : (clientsCount - 1);
	// 此处要用auto&，因为auto是值传递
	for (size_t i = begin; i <= end; i++)
		clients[i] = new EasyTcpClient;

	for (size_t i = begin; i <= end; i++)
	{
		clients[i]->Connect("192.168.0.109", 4567);
		printf("Thread %d Number: %u joined...\n", id, i);
	}

	readyNum++;
	while(readyNum < threadCount)
		std::this_thread::sleep_for(std::chrono::seconds(3));

	std::thread t(RecvThread, begin, end);

	while (true)
	{
		for (size_t i = begin; i <= end; i++)
		{

			if (clients[i]->SendData((DataHeader*)&login) != SOCKET_ERROR)
				sendNum++;	
		};
	}
}

int main()
{
	//EasyTcpClient client;
	//client.InitSocket();
	//client.Connect("127.0.0.1", 4567);
	//strcpy_s(login.userName, "Benson");
	//strcpy_s(login.password, "12345678");
	//std::thread t1(cmdThread, &client);
	//t1.detach();

	std::thread threads[threadCount];
	for (int i = 0; i < threadCount; i++)
	{
		threads[i] = std::thread(SendThread, i + 1);
	}

	CELLTimestamp clock;

	while (true)
	{
		double duration;
		if ((duration = clock.GetElapsedTimeInSecond()) >= 1.0)
		{
			printf("Clients: duration = %lf\tsendNum = %d\n", duration, sendNum.load());
			clock.Update();
			sendNum = 0;
		}
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	for (auto client : clients)
		client->Close();

	getchar();
	return 0;
}