#include <stdlib.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <memory>
#include "Allocator.h"
#include "CELLObjectPool.hpp"
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

class A : public ObjectPoolBase<A, 10>
{
public:
	A(int a) : _a(a) {}
private:
	int _a;
};

class B : public ObjectPoolBase<B, 20>
{
public:
	B(int a, int b) : _a(a), _b(b) {}
private:
	int _a;
	int _b;
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

	/*auto a = new int;
	std::shared_ptr<int> b = std::make_shared<int>();

	std::shared_ptr<Test> t = std::make_shared<Test>(10, 20);
	fun(t);*/

	auto a1 = new A(5);
	delete a1;

	auto a2 = A::createObject(5);

	A::destroyObject(a2);

	auto b = B::createObject(5, 6);
	B::destroyObject(b);

	return 0;
} 