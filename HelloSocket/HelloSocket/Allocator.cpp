#include "Allocator.h"
#include "MemoryMgr.hpp"

void* operator new(size_t size)
{
	return MemoryMgr::Instance().allocMem(size);
}

void operator delete(void* p)
{
	MemoryMgr::Instance().freeMem(p);
}

void* operator new[](size_t size)
{
	return MemoryMgr::Instance().allocMem(size);
}

void operator delete[](void* p)
{
	MemoryMgr::Instance().freeMem(p);
}