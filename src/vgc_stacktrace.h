//
// Copyright (C) 2016-2024 by Vincenzo Capuano
//
#pragma once
#include "vgc_malloc_private.h"

// Functions
//
void vgc_stacktraceSave(VGC_mallocHeader *mallocBlock);
void vgc_stacktraceShow(VGC_mallocHeader *mallocBlock);
bool vgc_stacktraceInit(void);
