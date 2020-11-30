/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. ①First version
						 
$$HISTORY$$
====================================================================================================*/
#ifndef _MEMORY_MGR_INCLUDED
#define _MEMORY_MGR_INCLUDED

#include <stdlib.h>

class MemoryAlloc;

// 内存块
struct MemoryBlock
{
	int nID;  // 内存块编号
	int nRef; // 引用次数
	MemoryAlloc* pAlloc = nullptr;  // 所属大内存块
	MemoryBlock* pNext;  // 下一个内存块
	bool inPool;
};

// 内存池 单例
class MemoryAlloc
{
public:
	MemoryAlloc() {}

	// 申请内存
	void* allocMem(size_t size)
	{
		return malloc(size);
	}

	// 释放
	void freeMem(void* p)
	{
		free(p);
	}

private:
	char* _pBuf = nullptr; // 内存池地址
	MemoryBlock* _pHeader = nullptr; // 内存池头部单元地址
	size_t _nSize = 0;
	size_t nBlockNum = 0;
	
};

// 内存管理工具
class MemoryMgr
{
public:
	// 申请内存
	void* allocMem(size_t size)
	{
		return malloc(size);
	}

	// 释放
	void freeMem(void* p)
	{
		free(p);
	}

	static MemoryMgr& Instance()
	{
		return mgr;
	}

private:

	static MemoryMgr mgr;

};

#endif
