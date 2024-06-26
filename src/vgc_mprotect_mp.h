//
// Copyright (C) 2024 by Vincenzo Capuano
//
#pragma once

#include "vgc_mprotect.h"


#if defined(VGC_MALLOC_MPROTECT_MP) && (defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY))
bool startMprotect(int maxProcesses);
void stopMprotect(void);
void startChildMprotect(void);
void stopChildMprotect(void);
void mprotectDistribute(VGC_mallocHeader *header, int prot);
#endif


