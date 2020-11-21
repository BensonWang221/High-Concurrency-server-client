/*==================================================================================================
   Date                        Description of Change
09-Nov-2020           First version
09-Nov-2020           ①针对Linux在遍历返回的fd_set时，不再每次遍历获取maxFd，而是保存maxFd，在新加或删除时更新maxFd
                      ②注意vector/deque在用iterator遍历时，不能随便erase，注意迭代器陷阱！erase后续iterator失效！！
14-Nov-2020           封装类EasyTcpServer，重写server.cpp

$$HISTORY$$
====================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "EasyTcpServer.hpp"


int main()
{
	EasyTcpServer server;
	//server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(3000);

	while (true)
	{
		server.OnRun();
	}

	server.Close();
	getchar();
	return 0;
}