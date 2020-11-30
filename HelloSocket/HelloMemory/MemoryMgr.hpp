/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. ��First version
						 
$$HISTORY$$
====================================================================================================*/
#ifndef _MEMORY_MGR_INCLUDED
#define _MEMORY_MGR_INCLUDED

#include <stdlib.h>

class MemoryAlloc;

// �ڴ��
struct MemoryBlock
{
	int nID;  // �ڴ����
	int nRef; // ���ô���
	MemoryAlloc* pAlloc = nullptr;  // �������ڴ��
	MemoryBlock* pNext;  // ��һ���ڴ��
	bool inPool;
};

// �ڴ�� ����
class MemoryAlloc
{
public:
	MemoryAlloc() {}

	// �����ڴ�
	void* allocMem(size_t size)
	{
		return malloc(size);
	}

	// �ͷ�
	void freeMem(void* p)
	{
		free(p);
	}

private:
	char* _pBuf = nullptr; // �ڴ�ص�ַ
	MemoryBlock* _pHeader = nullptr; // �ڴ��ͷ����Ԫ��ַ
	size_t _nSize = 0;
	size_t nBlockNum = 0;
	
};

// �ڴ������
class MemoryMgr
{
public:
	// �����ڴ�
	void* allocMem(size_t size)
	{
		return malloc(size);
	}

	// �ͷ�
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
