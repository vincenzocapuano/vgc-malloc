//
// Copyright (C) 2020-2024 by Vincenzo Capuano
//
#pragma once

#include "vgc_malloc_private.h"


#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
bool VGC_mprotect(VGC_mallocHeader *header);
bool VGC_munprotect(VGC_mallocHeader *header);
#endif


