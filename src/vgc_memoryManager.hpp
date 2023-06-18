#include <pthread.h>

// SOSTITUIRE CON I THREAD DEL C++

//
//Use memory manager:
//void ∗operator new(size_t size, MemoryManager& memMgr);
//void  operator delete(void∗ pointerToDelete, MemoryManager& memMgr);

class IMemoryManager
{
	public:
		virtual void *allocate(size_t size) = 0;
		virtual void  free(void* ptr) = 0;

	protected:
		pthread_mutex_t lock;
};


class MemoryManager: public IMemoryManager
{
	public:
		MemoryManager();
		virtual ~MemoryManager();
		virtual void *allocate(size_t size);
		virtual void  free(void* ptr);
};
