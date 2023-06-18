#include "vgc_memoryManager.hpp"


MemoryManager::MemoryManager()
{
	pthread_mutex_init(&lock, 0);
	// usual constructor code
}


MemoryManager::~MemoryManager()
{
	// usual destructor code
	pthread_mutex_destroy(&lock);
}


void *MemoryManager::allocate(size_t size)
{
	pthread_mutex_lock (&lock);
	// usual memory allocation code
	pthread_mutex_unlock (&lock);
	return 0;
}


void MemoryManager::free(void *ptr)
{
	pthread_mutex_lock (&lock);
	// usual memory deallocation code
	pthread_mutex_unlock (&lock);
}
