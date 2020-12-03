#include <stdlib.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <memory>
#include "Allocator.h"
using namespace std;

const int threadNum = 8;
const int memCount = 100000;
const int aveCount = memCount / threadNum;

void ThreadFun()
{
	char* data[aveCount];
	for (int i = 0; i < aveCount; ++i)
		data[i] = new char((rand() % 127) + 1);

	for (int i = 0; i < aveCount; i++)
		delete[] data[i];
}

class Test
{
public:
	Test(int a, int b)
	{
		cout << "Test()...\n";
	}

	~Test()
	{
		cout << "~Test()...\n";
	}
private:
};

void fun(std::shared_ptr<Test> a)
{
	std::shared_ptr<Test>& b = a;
	cout << "fun...\n";
}

int main()
{
	/*thread threads[threadNum];
	for (int i = 0; i < threadNum; i++)
		threads[i] = thread(ThreadFun);

	for (int i = 0; i < threadNum; i++)
		threads[i].join();*/

	auto a = new int;
	std::shared_ptr<int> b = std::make_shared<int>();

	std::shared_ptr<Test> t = std::make_shared<Test>(10, 20);
	fun(t);

	return 0;
} 