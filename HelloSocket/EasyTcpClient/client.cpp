/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           First version
09-Nov-2020           根据封装的EasyTcpClient类改进client应用程序
15-Nov-2020           测试粘包问题

$$HISTORY$$
====================================================================================================*/

#include "EasyTcpClient.hpp"
#include <thread>

char userName[32];


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

int main()
{
	EasyTcpClient client;
	client.InitSocket();
	client.Connect("127.0.0.1", 4567);

	//std::thread t1(cmdThread, &client);
	//t1.detach();

	Login login;
	strcpy_s(login.userName, "Benson");
	strcpy_s(login.password, "12345678");

	while (client.IsRunning())
	{
		client.OnRun();
		client.SendData((DataHeader*)&login);
	}

	client.Close();

	getchar();
	return 0;
}