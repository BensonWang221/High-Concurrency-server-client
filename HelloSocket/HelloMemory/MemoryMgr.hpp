/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. ��First version
01-Dec-2020           1. ʵ���ڴ��
						 
$$HISTORY$$
====================================================================================================*/
#ifndef _MEMORY_MGR_INCLUDED
#define _MEMORY_MGR_INCLUDED

#include <stdlib.h>
#include <assert.h>
#include <algorithm>

const size_t MAX_MEMORY_SIZE = 64;

class MemoryAlloc;

// �ڴ��
struct MemoryBlock
{
	int nID;  // �ڴ����
	int nRef; // ���ô���
	MemoryAlloc* pAlloc = nullptr;  // �������ڴ��
	MemoryBlock* pNext = nullptr;  // ��һ���ڴ��
	bool inPool;
};

// �ڴ�� ����
class MemoryAlloc
{
public:
	MemoryAlloc() = default;

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

	// �����ڴ�
	void* AllocMemory(size_t size)
	{
		if (!_pBuf)
			InitMemory();

		MemoryBlock* pResult = nullptr;

		if (_pHeader == nullptr)
			pResult = AllocOutOfPool(size);

		else
		{
			pResult = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(pResult->nRef == 0);
			pResult->nRef = 1;
		}
		return reinterpret_cast<char*>(pResult) + sizeof(MemoryBlock);
	}

	inline MemoryBlock* AllocOutOfPool(size_t size)
	{
		MemoryBlock* pResult = static_cast<MemoryBlock*>(malloc(size + sizeof(MemoryBlock)));
		pResult->inPool = false;
		pResult->nID = -1;
		pResult->nRef = 1;
		pResult->pAlloc = this;
		pResult->pNext = nullptr;

		return reinterpret_cast<MemoryBlock*>(reinterpret_cast<char*>(pResult) + sizeof(MemoryBlock));
	}

	// �ͷ�
	void FreeMemory(void* p)
	{
		MemoryBlock* pData = reinterpret_cast<MemoryBlock*>(static_cast<char*>(p) - sizeof(MemoryBlock));

		assert(pData->nRef == 1);

		if (--pData->nRef != 0)
			return;

		if (!pData->inPool)
			free(pData);
		else
		{
			// ��pData��Ϊheader�� ԭ��header��Ϊnext
			// ������֤header��header->pNext����Ϊ��
			// �����allocʱ����header�Ժ���Խ�headerֱ������Ϊ��next
			// ����header��Ϊ���һ��, �����Ժ�header��Ϊ��nextΪnullptr���´λ��жϳ���
			pData->pNext = _pHeader;
			_pHeader = pData;
		}
	}

	void InitMemory()
	{
		assert(_pBuf == nullptr);

		if (_pBuf != nullptr)
			return;

		size_t bufSize = _nSize * _nBlockNum;

		if (bufSize != 0)
			_pBuf = (char*)malloc(bufSize);

		if (_pBuf == nullptr)
			return;

		_pHeader = reinterpret_cast<MemoryBlock*>(_pBuf);

		MemoryBlock* pCurrent = _pHeader, *pPrevious = nullptr;
		for (size_t i = 0; i < _nBlockNum; i++)
		{
			pPrevious = pCurrent;
			pCurrent->pNext = reinterpret_cast<MemoryBlock*>(_pBuf + (i + 1) * _nSize);
			pCurrent->nID = i;
			pCurrent->inPool = true;
			pCurrent->nRef = 0;
			pCurrent->pAlloc = this;
			pCurrent = pCurrent->pNext;
		}
		if (!pPrevious)
			pPrevious->pNext = nullptr;
	}

private:
	char* _pBuf = nullptr; // �ڴ�ص�ַ
	MemoryBlock* _pHeader = nullptr; // �ڴ��ͷ����Ԫ��ַ
	size_t _nSize = 0;
	size_t _nBlockNum = 0;
	
};

// �ڴ������
class MemoryMgr
{
public:
	// �����ڴ�
	void* allocMem(size_t size)
	{
		if (size <= MAX_MEMORY_SIZE)
			return _memAllocs[size]->AllocMemory(size);

		else
			return _mem64.AllocOutOfPool(size);
	}

	// �ͷ�
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
	}

	void Init(size_t nBegin, size_t nEnd, MemoryAlloc* pMemAlloc)
	{
		std::for_each(_memAllocs + nBegin, _memAllocs + nEnd + 1, [pMemAlloc](MemoryAlloc*& pMem)
			{
				pMem = pMemAlloc;
			});
	}

	MemoryAlloc _mem64 = MemoryAlloc(64, 10);
	MemoryAlloc* _memAllocs[MAX_MEMORY_SIZE + 1];
};

#endif
