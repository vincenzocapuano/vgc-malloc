//
// Copyright (C) 2024 by Vincenzo Capuano
//
#pragma once

#include "vgc_malloc_private.h"
#include "vgc_mprotect.h"


#if defined(VGC_MALLOC_MPROTECT_MP) && (defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_PKEYMPROTECT))
bool startMprotect(int maxProcesses);
void stopMprotect(void);
void configMprotect(VGC_shared *shared);
void startChildMprotect(void);
void stopChildMprotect(void);
void mprotectDistribute(VGC_mallocHeader *header, int prot);
#else
# define configMprotect(a) /**/
# define startMprotect(a)  /**/
# define stopMprotect(a)   /**/
#endif


