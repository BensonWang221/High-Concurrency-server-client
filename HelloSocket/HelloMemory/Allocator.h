#ifndef _ALLOCATOR_INCLUDED
#define _ALLOCATOR_INCLUDED

void* operator new(size_t);

void operator delete(void*);

void* operator new[](size_t);

void operator delete[](void*);

#endif
