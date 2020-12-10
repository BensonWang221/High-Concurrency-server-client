/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. ①First version
01-Dec-2020           1. 实现内存池
						 
$$HISTORY$$
====================================================================================================*/
#ifndef _MEMORY_MGR_INCLUDED
#define _MEMORY_MGR_INCLUDED

#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include <mutex>

#ifdef _DEBUG
#include <iostream>
	#define xPrintf(...) printf(__VA_ARGS__)
#else
	#define xPrintf
#endif

const size_t MAX_MEMORY_SIZE = 128;

class MemoryAlloc;

// 内存块
struct MemoryBlock
{
	int nID;  // 内存块编号
	int nRef; // 引用次数
	MemoryAlloc* pAlloc = nullptr;  // 所属大内存块
	MemoryBlock* pNext = nullptr;  // 下一个内存块
	bool inPool;
};

int a = sizeof(MemoryBlock);

// 内存池 单例
class MemoryAlloc
{
public:
	MemoryAlloc() = default;

	MemoryAlloc(const MemoryAlloc&) = default;

	MemoryAlloc(size_t nSize, size_t nBlocks) : _nBlockNum(nBlocks) 
	{
		constexpr size_t ptrSize = sizeof(void*);
		_nSize = (nSize % ptrSize) ? (nSize / ptrSize + 1) * ptrSize : nSize;
	}

	~MemoryAlloc()
	{
		if (_pBuf)
			free(_pBuf);
	}

	// 申请内存
	void* AllocMemory(size_t size)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_pBuf)
			InitMemory();

		MemoryBlock* pResult = nullptr;

		if (_pHeader == nullptr)
		{
			//printf("AllocMemory: Run out of total memory pool, size = %d\n", size);
			return AllocOutOfPool(size);
		}

		pResult = _pHeader;
		_pHeader = _pHeader->pNext;
		assert(pResult->nRef == 0);
		pResult->nRef = 1;

		//xPrintf("AllocMemory:  %p, id = %d, size = %d\n", pResult, pResult->nID, size);

		return reinterpret_cast<char*>(pResult) + sizeof(MemoryBlock);
	}

	inline void* AllocOutOfPool(size_t size)
	{
		MemoryBlock* pResult = static_cast<MemoryBlock*>(malloc(size + sizeof(MemoryBlock)));
		pResult->inPool = false;
		pResult->nID = -1;
		pResult->nRef = 1;
		pResult->pAlloc = this;
		pResult->pNext = nullptr;

		//xPrintf("AllocOutOfPool: %p, id = %d, size = %d\n", pResult, pResult->nID, size);

		return reinterpret_cast<char*>(pResult) + sizeof(MemoryBlock);
	}

	// 释放
	void FreeMemory(void* p)
	{
		MemoryBlock* pData = reinterpret_cast<MemoryBlock*>(static_cast<char*>(p) - sizeof(MemoryBlock));

		//xPrintf("FreeMemory:  %p, id = %d\n", pData, pData->nID);

		assert(pData->nRef == 1);

		if (--pData->nRef != 0)
			return;

		if (!pData->inPool)
			free(pData);
		else
		{
			std::lock_guard<std::mutex> lock(_mutex);
			// 将pData设为header， 原有header设为next
			// 这样保证header和header->pNext都不为空
			// 因此在alloc时拿走header以后可以将header直接设置为其next
			// 除非header已为最后一个, 拿走以后header变为其next为nullptr，下次会判断出来
			pData->pNext = _pHeader;
			_pHeader = pData;
		}
	}

	void InitMemory()
	{
		//xPrintf("Init MemoryAlloc: size = %d\n", this->_nSize);
		assert(_pBuf == nullptr);

		if (_pBuf != nullptr)
			return;

		size_t bufSize = (_nSize + sizeof(MemoryBlock)) * _nBlockNum;

		if (bufSize != 0)
			_pBuf = (char*)malloc(bufSize);

		if (_pBuf == nullptr)
			return;

		_pHeader = reinterpret_cast<MemoryBlock*>(_pBuf);

		MemoryBlock* pCurrent = _pHeader, *pPrevious = nullptr;
		for (size_t i = 0; i < _nBlockNum; i++)
		{
			pPrevious = pCurrent;
			pCurrent->pNext = reinterpret_cast<MemoryBlock*>(_pBuf + (i + 1) * (_nSize + sizeof(MemoryBlock)));
			pCurrent->nID = i;
			pCurrent->inPool = true;
			pCurrent->nRef = 0;
			pCurrent->pAlloc = this;
			pCurrent = pCurrent->pNext;
		}
		if (pPrevious)
			pPrevious->pNext = nullptr;
	}

protected:
	char* _pBuf = nullptr; // 内存池地址
	MemoryBlock* _pHeader = nullptr; // 内存池头部单元地址
	size_t _nSize = 0;
	size_t _nBlockNum = 0;
	std::mutex _mutex;
};

template <size_t nSize, size_t nBlockNum>
class MemoryAllocator : public MemoryAlloc
{
public:
	MemoryAllocator() : MemoryAlloc(nSize, nBlockNum) {}
};

// 内存管理工具
class MemoryMgr
{
public:
	// 申请内存
	void* allocMem(size_t size)
	{
		if (size <= MAX_MEMORY_SIZE)
			return _memAllocs[size]->AllocMemory(size);

		else
			return _mem64.AllocOutOfPool(size);
	}

	// 释放
	void freeMem(void* p)
	{
		MemoryBlock* pData = reinterpret_cast<MemoryBlock*>(static_cast<char*>(p) - sizeof(MemoryBlock));
		pData->pAlloc->FreeMemory(p);
	}

	static MemoryMgr& Instance()
	{
		static MemoryMgr mgr;
		return mgr;
	}

private:
	MemoryMgr()
	{
		_mem64.InitMemory();
		Init(1, 64, &_mem64);
		_mem128.InitMemory();
		Init(65, 128, &_mem128);
		//Init(129, 256, &_mem256);
		//Init(257, 512, &_mem512);
		//Init(513, 1024, &_mem1024);
	}

	void Init(size_t nBegin, size_t nEnd, MemoryAlloc* pMemAlloc)
	{
		std::for_each(_memAllocs + nBegin, _memAllocs + nEnd + 1, [pMemAlloc](MemoryAlloc*& pMem)
			{
				pMem = pMemAlloc;
			});
	}

	// 当MemoryAlloc里含有不支持拷贝的成员_mutex时，不能像这样在MemoryMgr里初始化MemoryAlloc成员
	// 解决办法一：在MemoryMgr的构造函数里，一个个给已经调用默认无参构造函数构造完成的MemoryAlloc成员改变其_nSize和_nBlockNum
	// 解决办法二: 利用模板函数，在编译时实例化从而定制出对应参数的默认无参构造函数
	/*MemoryAlloc _mem64 = MemoryAlloc(64, 1000000);
	MemoryAlloc _mem128 = MemoryAlloc(128, 1000000);
	MemoryAlloc _mem256 = MemoryAlloc(256, 100);
	MemoryAlloc _mem512 = MemoryAlloc(512, 100);
	MemoryAlloc _mem1024 = MemoryAlloc(1024, 100);*/

	MemoryAllocator<64, 4000000> _mem64;
	MemoryAllocator<128, 1000000> _mem128;
	//MemoryAllocator<256, 100000> _mem256;
	//MemoryAllocator<512, 100000> _mem512;
	//MemoryAllocator<1024, 100000> _mem1024;

	MemoryAlloc* _memAllocs[MAX_MEMORY_SIZE + 1];
};

#endif
