//
// Copyright (C) 2020-2023 by Vincenzo Capuano
//
#pragma once

#include "vgc_malloc_private.h"

#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_PKEYMPROTECT)
bool VGC_mprotect(VGC_mallocHeader *header);
bool VGC_munprotect(VGC_mallocHeader *header);
bool startMprotect(int maxProcesses);
void stopMprotect(void);
void configMprotect(VGC_shared *shared);
void startChildMprotect(void);
void stopChildMprotect(void);
#else
# define VGC_mprotect(a)   /**/
# define VGC_munprotect(a) /**/
# define configMprotect(a) /**/
# define startMprotect(a)  /**/
# define stopMprotect(a)   /**/
#endif

