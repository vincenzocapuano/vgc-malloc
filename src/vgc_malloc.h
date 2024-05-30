//
// Copyright (C) 2018-2024 by Vincenzo Capuano
//
#pragma once
#include <string.h>

#ifdef __cplusplus
extern "C" {
	namespace vgc {
#endif

void *vgc_malloc(size_t size);
void *vgc_calloc(size_t nmemb, size_t size);
void *vgc_realloc(void *ptr, size_t size);
void  vgc_free(void *ptr);

#ifdef __cplusplus
	}
}
#endif
