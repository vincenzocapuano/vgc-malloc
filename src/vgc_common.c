#include "vgc_common.h"

// Returns the maximum between 2 values
//
ATTR_PUBLIC size_t vgc_max(size_t a, size_t b)
{
	return a > b ? a : b;
}


// Returns the minimum between 2 values
//
ATTR_PUBLIC size_t vgc_min(size_t a, size_t b)
{
	return a < b ? a : b;
}
