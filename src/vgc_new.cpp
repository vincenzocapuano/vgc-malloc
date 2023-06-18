#include <cstdlib>
#include <iostream>

#include "vgc_malloc.h"
#include "vgc_new.hpp"


void* operator new(std::size_t size)
{
	return vgc::vgc_malloc(size);
}


void operator delete(void* ptr) noexcept
{
	vgc::vgc_free(ptr);
}
