#include <stdlib.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <memory>
#include "Allocator.h"
#include "CELLObjectPool.hpp"
using namespace std;

const int threadNum = 4;
const int memCount = 16;
const int aveCount = memCount / threadNum;

class A : public ObjectPoolBase<A, 5>
{
public:
	A(int a) : _a(a) 
	{
		cout << "A(int)...\n";
	}

	~A()
	{
		cout << "~A()...\n";
	}

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

void ThreadFun()
{
	A* data[aveCount];
	for (int i = 0; i < aveCount; ++i)
		data[i] = A::createObject(5);

	for (int i = 0; i < aveCount; i++)
		A::destroyObject(data[i]);
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

	/*auto a = new int;
	std::shared_ptr<int> b = std::make_shared<int>();

	std::shared_ptr<Test> t = std::make_shared<Test>(10, 20);
	fun(t);

	/*auto a1 = new A(5);
	delete a1;

	auto a2 = A::createObject(5);

	A::destroyObject(a2);

	auto b = B::createObject(5, 6);
	B::destroyObject(b);*/

	A* a1 = new A(10);
	delete a1;

	{
		std::shared_ptr<A> a2(new A(10));
	}
	std::cout << "---------------\n";
	std::shared_ptr<A> a3(new A(10));;

	return 0;
} 