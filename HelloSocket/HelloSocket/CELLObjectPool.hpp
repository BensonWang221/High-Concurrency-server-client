/*==================================================================================================
   Date                        Description of Change
05-Dec-2020           1. ①First version
					     ②实现对象池
05-Dec-2020           1. 智能指针在搭配对象池时，不能使用std::make_shared

$$HISTORY$$
====================================================================================================*/
#ifndef _CELLOBJECTPOOL_INCLUDED
#define _CELLOBJECTPOOL_INCLUDED

#include <stdlib.h>
#include <mutex>
#include <assert.h>

#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif
#else
	#ifndef _xPrintf
		#define xPrintf(...) 
	#endif
#endif

template <class Type, size_t nPoolNum>
class CELLObjectPool
{
public:
	CELLObjectPool()
	{
		initPool();
	}

	~CELLObjectPool()
	{
		if (_pBuf)
			delete[] _pBuf;
	}

	void* allocObject(size_t nSize)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_pBuf)
			initPool();

		if (_pHeader == nullptr)
			return allocOutOfPool();

		NodeHeader* result = _pHeader;
		result->nRef = 1;
		_pHeader = _pHeader->pNext;

		printf("allocObject: size = %d, id = %d\n", nSize, result->nID);

		return reinterpret_cast<char*>(result) + sizeof(NodeHeader);
	}

	inline void* allocOutOfPool()
	{
		auto header = reinterpret_cast<NodeHeader*>(new char[sizeof(Type) + sizeof(NodeHeader)]);
		header->nID = -1;
		header->nRef = 1;
		header->inPool = false;
		header->pNext = nullptr;

		//xPrintf("Object Pool: allocOutOfPool\n");

		return reinterpret_cast<char*>(header) + sizeof(NodeHeader);
	}

	void freeObject(void* pObj)
	{
		auto p = reinterpret_cast<NodeHeader*>(static_cast<char*>(pObj) - sizeof(NodeHeader));
		
		std::lock_guard<std::mutex> lock(_mutex);
		if (p->inPool)
		{
			assert(p->nRef == 1);
			if (--p->nRef != 0)
			{
				printf("free object in pool but nRef isn't 1...\n");
			}
			p->pNext = _pHeader;
			_pHeader = p;
			p->nRef = 0;
		}
		else
			delete[] p;
	}

private:
	struct NodeHeader;

	void initPool()
	{
		if (_pBuf)
			return;

		size_t blockSize = (sizeof(Type) + sizeof(NodeHeader));
		_pBuf = new char[blockSize * nPoolNum];
		if (!_pBuf)
			return;

		_pHeader = reinterpret_cast<NodeHeader*>(_pBuf);


		NodeHeader* currentNode = _pHeader, * previousNode = nullptr;
		for (size_t i = 0; i < nPoolNum; i++)
		{
			previousNode = currentNode;
			currentNode->pNext = reinterpret_cast<NodeHeader*>(_pBuf + (i + 1) * blockSize);
			currentNode->nID = i;
			currentNode->nRef = 0;
			currentNode->inPool = true;
			currentNode = currentNode->pNext;
		}
		if (previousNode)
			previousNode->pNext = nullptr;
	}

	NodeHeader* _pHeader = nullptr;
	char* _pBuf = nullptr;
	std::mutex _mutex;
};

template <class Type, size_t nPoolNum>
struct CELLObjectPool<Type, nPoolNum>::NodeHeader
{
	NodeHeader* pNext = nullptr;
	int nID;
	char nRef;
	bool inPool;
};

template <class T, size_t nPoolNum>
class ObjectPoolBase
{
public:
	void* operator new(size_t nSize)
	{
		return objectPool().allocObject(nSize);
	}

	void operator delete(void* p)
	{
		objectPool().freeObject(p);
	}

	template <class... Args>
	static T* createObject(Args...args)
	{
		return new T(args...);
	}

	static void destroyObject(T* obj)
	{
		delete obj;
	}

private:
	using ClassTPool = CELLObjectPool<T, nPoolNum>;
	static ClassTPool& objectPool()
	{
		static ClassTPool sPool;

		return sPool;
	}
};

#endif