//
// Copyright (C) 2015-2024 by Vincenzo Capuano
//
#pragma once

#include "vgc_pthread.h"


// Defines
//
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
#define VGC_MALLOC_SYSTEM_PAGE_SIZE 4096
#endif
#ifdef VGC_MALLOC_STACKTRACE
#define VGC_MALLOC_STACKTRACE_SIZE 10
#endif


// Malloc memory block status
//
typedef enum {
	VGC_MALLOC_FREE,
	VGC_MALLOC_BUSY
} VGC_mallocStatus;


// MMAP header block
//
typedef struct VGC_mmapHeader {
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
	union {
		char                           alignBuffer[VGC_MALLOC_SYSTEM_PAGE_SIZE];
		struct {
#endif
			unsigned char          checkStart;
			size_t                 size;
			size_t                 maxSize;
			size_t                 free;
			size_t                 elements;	// Number of malloc's active on this MMAP
			pthread_mutex_t        mutex;
			pthread_mutexattr_t    mutexAttr;
			struct VGC_mmapHeader *prev;
			struct VGC_mmapHeader *next;
			unsigned char          checkEnd;
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
		};
	};
#endif
} VGC_mmapHeader;


// Malloc header block
//
typedef struct VGC_mallocHeader {
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
	union {
		char                             alignBuffer[VGC_MALLOC_SYSTEM_PAGE_SIZE * 2];
		struct {
			char                     protect[VGC_MALLOC_SYSTEM_PAGE_SIZE];
#ifdef VGC_MALLOC_MPROTECT_PKEY
			int			 pkey;
#endif
#endif
			unsigned char            checkStart;
			size_t                   size;
			VGC_mallocStatus         status;
			struct VGC_mmapHeader   *mmapBlock;
			struct VGC_mallocHeader *prev;
			struct VGC_mallocHeader *next;
			unsigned char            checkEnd;
#ifdef VGC_MALLOC_STACKTRACE
			void			*btArray[VGC_MALLOC_STACKTRACE_SIZE];
			size_t			 btArraySize;
#endif
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
		};
	};
#endif
} VGC_mallocHeader;


// All the malloc management data is here
//
#if defined(VGC_MALLOC_MPROTECT_MP) && (defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY))
# define SocketNameSize 100

typedef struct VGC_mallocDebugChild {
	pid_t        pid;
	pthread_t    thread;
	int          socketFD;
	char         socketName[SocketNameSize];
} VGC_mallocDebugChild;
#endif

typedef struct VGC_shared {
	pid_t		      pid;
	pthread_mutex_t       mutex;
	pthread_mutexattr_t   mutexAttr;
	size_t                pageSize;
	VGC_mmapHeader       *mmapBlockFirst;
	int                   mmapBlockCount;
	size_t		      mmapBlockSize;		// Number of pages of 4kB (_SC_PAGE_SIZE) to allocate at each call of MMAP
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
	bool		      isMprotectEnabled;
#  if defined(VGC_MALLOC_MPROTECT_MP)
	int                   maxProcesses;
	bool                  isFather;
	VGC_mallocDebugChild *children;
#  endif
#endif
} VGC_shared;
