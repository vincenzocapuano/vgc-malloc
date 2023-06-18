//
// Copyright (C) 2020 by Vincenzo Capuano
//
#pragma once

#include "vgc_malloc_private.h"

#ifdef VGC_MALLOC_MPROTECT
bool VGC_mprotect(VGC_mallocHeader *header);
bool VGC_munprotect(VGC_mallocHeader *header);
#else
# define VGC_mprotect(a)   /**/
# define VGC_munprotect(a) /**/
#endif

bool startMprotect(int maxProcesses);
void stopMprotect(void);
void configMprotect(VGC_shared *shared);
void startChildMprotect(void);
void stopChildMprotect(void);
