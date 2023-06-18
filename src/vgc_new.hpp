#include <cstdlib>
#include <new>

void* operator new(std::size_t size);
void  operator delete(void* ptr) noexcept;
