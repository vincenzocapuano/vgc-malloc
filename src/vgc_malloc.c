//
// Copyright (C) 2015-2024 by Vincenzo Capuano
//
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/param.h>

#include "vgc_common.h"
#include "vgc_message.h"
#include "vgc_stacktrace.h"
#include "vgc_mprotect.h"
#include "vgc_mprotect_mp.h"
#include "vgc_malloc_private.h"
#include "vgc_malloc.h"


#ifndef VGC_MALLOC_DEBUG_LEVEL
# define VGC_MALLOC_DEBUG_LEVEL 4
#endif

#ifndef VGC_MALLOC_MMAP_PAGES
# define VGC_MALLOC_MMAP_PAGES 8000
#endif

static void mallocCleanup(void);
static bool initializeShared(void);

// All the malloc management data is here
//
VGC_shared *shared = 0;


static const char *moduleName = "VGC-MALLOC";


// Executed once at library loading
//
static void __attribute__ ((constructor)) my_init(void)
{
	pid_t master = getpid();
	printf("Starting.....................: %d\n", master);
	vgc_messageInit();
	if (!vgc_stacktraceInit()) exit(EXIT_FAILURE);	// Used by mprotect
	if (!initializeShared()) exit(EXIT_FAILURE);
	shared->pid = master;
}

// Executed every time the library is released by a process
//
static void __attribute__ ((destructor)) my_fini(void)
{
	// This function is called every time a process using this library ends
	// we want to close everything only when the library is unloaded
	//
	pid_t master = getpid();
	if (master == shared->pid) {
		mallocCleanup();
		printf("Stopping.....................: %d\n", master);
	}
}


// firstMallocHeaderInMMAP
//
static inline VGC_mallocHeader *firstMallocHeaderInMMAP(VGC_mmapHeader *mmapBlock)
{
	return (VGC_mallocHeader*)((char*)mmapBlock + sizeof(VGC_mmapHeader));
}


// dumpMmapBlock
//
// Must be inside a mutex for the mmapBlock
//
static void dumpMmapBlock(char *response, size_t size, const char *str, VGC_mmapHeader *mmapBlock, const char *desc)
{
	if (mmapBlock == 0 || str == 0 || desc == 0) return;

	const char *dashes =  "---------------------------------------------------------------------";

	if (response != 0) {
		snprintf(&response[strlen(response)], mmapBlock->maxSize, "\nMMAP at 0x%.12lx\n", (long unsigned int)mmapBlock);
		snprintf(&response[strlen(response)], mmapBlock->maxSize, " ID memory         prev           next           status    size\n");
		snprintf(&response[strlen(response)], mmapBlock->maxSize, "%s\n",dashes);
	}
	else {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, desc, 0, "MMAP at 0x%.12lx", mmapBlock);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, " ID memory         prev           next           status    size", "", 0);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, dashes, "", 0);
	}

	int i = 0;
	size_t busySize = 0;
	size_t freeSize = 0;
	size_t headersSize = 0;

	for (VGC_mallocHeader *mallocBlock = firstMallocHeaderInMMAP(mmapBlock); mallocBlock != 0; mallocBlock = mallocBlock->next) {
		headersSize += sizeof(VGC_mallocHeader);
		if (mallocBlock->status == VGC_MALLOC_FREE) {
			freeSize += mallocBlock->size;
		}
		else {
			busySize += mallocBlock->size;
		}

		if (response != 0) {
			char buffer[200];
			snprintf(buffer, 200, "%s%3d%s 0x%.12lx%s 0x%.12lx 0x%.12lx %s%s%s %9d bytes\n",
					c_blue, i++, c_black,
					(long unsigned int)((char*)mallocBlock + sizeof(VGC_mallocHeader)), c_red,
					mallocBlock->prev == 0 ? 0 : (long unsigned int)((char*)mallocBlock->prev + sizeof(VGC_mallocHeader)),
					mallocBlock->next == 0 ? 0 : (long unsigned int)((char*)mallocBlock->next + sizeof(VGC_mallocHeader)),
					mallocBlock->status == VGC_MALLOC_FREE ? c_green : c_blue, mallocBlock->status == VGC_MALLOC_FREE ? "FREE" : "BUSY", c_black,
					(int)mallocBlock->size);
			int length = strlen(response);
			if (length + strlen(buffer) >= size) return;
			strcat(&response[length], buffer);
		}
		else {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "", "", "%s%3d%s 0x%.12lx%s 0x%.12lx 0x%.12lx %s%s%s %9d bytes",
					c_blue, i++, c_black,
					(char*)mallocBlock + sizeof(VGC_mallocHeader), c_red,
					mallocBlock->prev == 0 ? 0 : (char*)mallocBlock->prev + sizeof(VGC_mallocHeader),
					mallocBlock->next == 0 ? 0 : (char*)mallocBlock->next + sizeof(VGC_mallocHeader),
					mallocBlock->status == VGC_MALLOC_FREE ? c_green : c_blue, mallocBlock->status == VGC_MALLOC_FREE ? "FREE" : "BUSY", c_black,
					mallocBlock->size);
		}

		if (mallocBlock->status == VGC_MALLOC_BUSY) {
			vgc_stacktraceShow(mallocBlock);
		}
	}

	size_t maxSize = shared->mmapBlockSize - sizeof(VGC_mmapHeader) - headersSize;
	size_t totalSize = busySize + freeSize;

	if (response != 0) {
		char buffer[100];

		snprintf(buffer, 100, "%s\n", dashes);
		int length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);

		snprintf(buffer, 100, "%ssize allocated.: %s%9d bytes\n", c_black, c_blue, (int)shared->mmapBlockSize);
		length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);

		snprintf(buffer, 100, "%ssize busy......: %s%9d bytes\n", c_black, c_blue, (int)busySize);
		length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);

		snprintf(buffer, 100, "%ssize free......: %s%9d bytes\n", c_black, c_blue, (int)freeSize);
		length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);

		snprintf(buffer, 100, "%ssize total.....: %s%9d bytes *\n", c_black, c_green, (int)totalSize);
		length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);

		snprintf(buffer, 100, "%ssize max.......: %s%9d bytes *\n", c_black, c_green, (int)maxSize);
		length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);

		snprintf(buffer, 100, "%s%s\n", c_black, dashes);
		length = strlen(response);
		if (length + strlen(buffer) >= size) return;
		strcat(&response[length], buffer);
	}
	else {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, dashes, "", 0);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "", "", "size allocated.: %s%9d bytes", c_blue, shared->mmapBlockSize);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "", "", "size busy......: %s%9d bytes", c_blue, busySize);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "", "", "size free......: %s%9d bytes", c_blue, freeSize);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "", "", "size total.....: %s%9d bytes *", c_green, totalSize);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "", "", "size max.......: %s%9d bytes *", c_green, maxSize);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, dashes, "", 0);
	}
}


// checkMmapBlock
//
// Must be inside a mutex for the mmapBlock
//
static bool checkMmapBlock(const char *str, VGC_mmapHeader *mmapBlock)
{
	size_t busySize = 0;
	size_t freeSize = 0;
	size_t headersSize = 0;

	for (register VGC_mallocHeader *mallocBlock = firstMallocHeaderInMMAP(mmapBlock); mallocBlock != 0; mallocBlock = mallocBlock->next) {
		headersSize += sizeof(VGC_mallocHeader);
		if (mallocBlock->status == VGC_MALLOC_FREE) {
			freeSize += mallocBlock->size;
		}
		else {
			busySize += mallocBlock->size;
		}

		if (mallocBlock->size > mmapBlock->maxSize) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "Found too big memory block", "Error", ": in memory allocated at 0x%.12lx", (char*)mallocBlock + sizeof(VGC_mallocHeader));
			dumpMmapBlock(0, 0, str, mmapBlock, "memory block wrong size");
			return false;
		}

		if (mallocBlock < (VGC_mallocHeader *)mmapBlock || mallocBlock > (VGC_mallocHeader *)((char *)mmapBlock + shared->mmapBlockSize)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "Found wrong memory pointer", "Error", ": in memory allocated at 0x%.12lx points outside the MMAP", (char*)mallocBlock + sizeof(VGC_mallocHeader));
			dumpMmapBlock(0, 0, str, mmapBlock, "memory block wrong pointer");
			return false;
		}

		if (mallocBlock->next == 0) {
			break;
		}

		if (mallocBlock != mallocBlock->next->prev) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "Found memory overwrite", "Error", ": in memory allocated at 0x%.12lx", (char*)mallocBlock + sizeof(VGC_mallocHeader));
			dumpMmapBlock(0, 0, str, mmapBlock, "memory block overwrite");
			return false;
		}
	}

	size_t maxSize = shared->mmapBlockSize - sizeof(VGC_mmapHeader) - headersSize;
	size_t totalSize = busySize + freeSize;

	if (maxSize != totalSize) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "Found inconsistent MMAP", "Error", ": in MMAP allocated at 0x%.12lx", mmapBlock);
		dumpMmapBlock(0, 0, str, mmapBlock, "MMAP block inconsistent");
	}

	return true;
}


// getMmapBlock
//
// Must be inside a mutex for the mmapBlock
//
static bool isInMmapBlock(void *ptr)
{
	for (register VGC_mmapHeader *mmapBlock = shared->mmapBlockFirst; mmapBlock != 0; mmapBlock = mmapBlock->next) {
		if (ptr > (void*)mmapBlock && ptr < (void*)((char *)mmapBlock + shared->mmapBlockSize)) return true;
	}

	return false;
}


// mallocCleanup
//
// Cleanup vgc_malloc variables
// It is called at the end of the main process
//
static void mallocCleanup(void)
{
	if (shared == 0) return;

	if (shared->mmapBlockFirst != 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mmapBlockFirst", "Block is not empty", 0, "memory leak%s list", shared->mmapBlockFirst->elements > 1 ? "s" : "");
		dumpMmapBlock(0, 0, __func__, shared->mmapBlockFirst, "Block is not empty");
	}

	if (!PTHREAD_mutexattrDestroy(&shared->mutexAttr)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexattrDestroy", "Error", "can't destroy shared mutexAttr", 0);
	}

	if (!PTHREAD_mutexDestroy(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexDestroy", "Error", "can't destroy shared mutexAttr", 0);
	}

#if defined(VGC_MALLOC_MPROTECT_MP)
	if (shared->isMprotectEnabled) stopMprotect();
#endif

	if (munmap(shared, sizeof(VGC_shared)) == -1) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP (shared)", ": %s", strerror(errno));
	}
}


// VGC_mmapHeader
//
static VGC_mmapHeader *allocMMAP(size_t mmapBlockSize, VGC_mmapHeader *mmapLastBlock)
{
	VGC_mmapHeader *mmapBlock = mmap(0, mmapBlockSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mmapBlock == MAP_FAILED) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mmap", "Error", "no more memory available", ": %s", strerror(errno));
		return MAP_FAILED;
	}

	// Initialise locking the MMAP block
	//
	if (!PTHREAD_mutexattrInit(&mmapBlock->mutexAttr)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexattrInit", "Error", "mmapBlock mutex attr init failed", 0);
		if (munmap(mmapBlock, mmapBlock->size) == -1) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP", ": %s", strerror(errno));
		}
		return MAP_FAILED;
	}
	if (!PTHREAD_mutexInit(&mmapBlock->mutex, &mmapBlock->mutexAttr)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexInit", "Error", "can't create mutex on mmapBlock", 0);
		if (!PTHREAD_mutexattrDestroy(&mmapBlock->mutexAttr)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexattrDestroy", "Error", "can't destroy mutex on mmapBlock", 0);
		}
		if (munmap(mmapBlock, mmapBlock->size) == -1) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP", ": %s", strerror(errno));
		}
		return MAP_FAILED;
	}

	if (!PTHREAD_mutexLock(&mmapBlock->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on mmapBlock", 0);
		if (munmap(mmapBlock, mmapBlock->size) == -1) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP", ": %s", strerror(errno));
		}
		return MAP_FAILED;
	}

	// Initialise the MMAP
	//
	mmapBlock->size = mmapBlockSize;
	mmapBlock->maxSize = mmapBlock->size - sizeof(VGC_mmapHeader) - sizeof(VGC_mallocHeader);
	mmapBlock->prev = mmapLastBlock;
	mmapBlock->next = 0;
	if (mmapLastBlock != 0) mmapLastBlock->next = mmapBlock;
	mmapBlock->elements = 0;
	mmapBlock->checkStart = 0xAA;
	mmapBlock->checkEnd = 0xAA;

	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, "vgc_malloc", "New MMAP at", "memory address", 0, "0x%lx (size: %dKB)", mmapBlock, mmapBlock->size / 1024);

	// Allocate the first malloc element in the map
	// The first element is a FREE block that takes all the available memory
	//
	VGC_mallocHeader *mallocBlock = (VGC_mallocHeader*)((char*)mmapBlock + sizeof(VGC_mmapHeader));
	mallocBlock->size = mmapBlock->maxSize;
	mallocBlock->status = VGC_MALLOC_FREE;
	mallocBlock->mmapBlock = mmapBlock;
	mallocBlock->prev = 0;
	mallocBlock->next = 0;
	mallocBlock->checkStart = 0xAA;
	mallocBlock->checkEnd = 0xAA;
	VGC_mprotect(mallocBlock);

	if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
		if (munmap(mmapBlock, mmapBlock->size) == -1) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP", ": %s", strerror(errno));
		}
		return MAP_FAILED;
	}

	// Increment MMAP counter
	//
	shared->mmapBlockCount++;
	return mmapBlock;
}


// allocMallocBlock
//
static void *allocMallocBlock(VGC_mmapHeader *mmapBlock, size_t length)
{
	// Allocate "length" space
	//
	if (!PTHREAD_mutexLock(&mmapBlock->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on mmapBlock", 0);
		return 0;
	}
	VGC_mallocHeader *mallocBlock = 0;
	for (mallocBlock = (VGC_mallocHeader*)((char*)mmapBlock + sizeof(VGC_mmapHeader)); mallocBlock != 0; mallocBlock = mallocBlock->next) {
		if (mallocBlock->status == VGC_MALLOC_FREE && mallocBlock->size >= length) break;
	}

	if (mallocBlock == 0) {
		// There is no space for the requested memory length in this MMAP block
		//
		if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
		}
		return 0;
	}

	if (!checkMmapBlock("vgc_malloc", mmapBlock)) {
		// mmapBlock is corrupted
		//
		if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
		}
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "checkMmapBlock", "Error", "The MMAP for vgc_malloc is unstable while allocating memory", 0);
		return 0;
	}

	int lengthOrig = length;
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
	if (shared->isMprotectEnabled) {
		length = (length % shared->pageSize == 0) ? length : (length / shared->pageSize + 1) * shared->pageSize;
	}
#endif

	// Next is the remaining free space
	// Is there any free space remaining in the block?
	//
	VGC_mallocHeader *next = mallocBlock->next;
	size_t blockSize = length + sizeof(VGC_mallocHeader);
	if (mallocBlock->size > blockSize) {
		next = (VGC_mallocHeader*)((char*)mallocBlock + blockSize);
		next->mmapBlock = mmapBlock;
		next->size = mallocBlock->size - blockSize;
		next->status = VGC_MALLOC_FREE;
		next->prev = mallocBlock;
		next->next = mallocBlock->next;
		next->checkStart = 0xAA;
		next->checkEnd = 0xAA;
		VGC_mprotect(next);
	}
	else {
		// No additional space in the block for the header and at least one byte more
		// Assign the full size to this block even if redundant
		//
		length = mallocBlock->size;
	}

	// Allocate the required space and return it
	//
	mmapBlock->elements++;
	mallocBlock->mmapBlock = mmapBlock;
	mallocBlock->size = length;
	mallocBlock->status = VGC_MALLOC_BUSY;
	mallocBlock->next = next;
	if (next && next->next) next->next->prev = next;
	mallocBlock->checkStart = 0xAA;
	mallocBlock->checkEnd = 0xAA;

	void *memory = (char*)mallocBlock + sizeof(VGC_mallocHeader);
	if (shared->isMprotectEnabled) {
		memory += (unsigned int)shared->pageSize - lengthOrig % shared->pageSize;
	}

	vgc_stacktraceSave(mallocBlock);
	if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
	}

	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, "vgc_malloc", "Malloc", "", "", "%d bytes at 0x%lx (#%u)", length, memory, mmapBlock->elements);

	if (shared->isMprotectEnabled) {
		vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, "vgc_malloc", "Protect", "", "", "protect:0x%lx - malloc:0x%lx-0x%lx (%s) - size:%d - pid:%u", mallocBlock, memory, (char*)memory + mallocBlock->size, mallocBlock->status == VGC_MALLOC_FREE ? "free" : "busy", mallocBlock->size, getpid());
	}

	return memory;
}


// initialiseMutex
//
static bool initialiseMutex(void)
{
	if (!PTHREAD_mutexattrInit(&shared->mutexAttr)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexattrInit", "Fatal error", "mutex attr init failed", 0);
		return false;
	}
	if (!PTHREAD_mutexInit(&shared->mutex, &shared->mutexAttr)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexInit", "Fatal error", "can't create shared mutex", 0);
		if (!PTHREAD_mutexattrDestroy(&shared->mutexAttr)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexattrDestroy", "Fatal error", "can't destroy shared attr mutex", 0);
		}
		return false;
	}

	return true;
}


// mmapBlockAllocate
//
static VGC_mmapHeader *mmapBlockAllocate(size_t mmapBlockSize, VGC_mmapHeader *mmapLastBlock)
{
	// Allocate first MMAP block
	//
	VGC_mmapHeader *mmapBlock = allocMMAP(mmapBlockSize, mmapLastBlock);
	if (mmapBlock == MAP_FAILED) {
		// No more space available in the system
		// Try allocating a smaller MMAP block of 1/10 of the original request
		//
		mmapBlock = allocMMAP(mmapBlockSize / 10, mmapLastBlock);
		if (mmapBlock == MAP_FAILED) {
			// Even the request for a smaller MMAP block failed,
			// there is definitely no more space available for this process in the system
			//
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "allocMMAP", "Error", "can't get enough memory for MMAP", ": %s", strerror(errno));
		}
	}
	return mmapBlock;
}


// createShared
//
static VGC_shared *createShared(void)
{
	VGC_shared *s = mmap(0, sizeof(VGC_shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (s == 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mmap", "Fatal error", "can't create shared MMAP memory for VGC_shared", ": %s", strerror(errno));
		return 0;
	}

	// Initialise page size, MMAP block size and MMAP block count
	//
	s->pageSize = sysconf(_SC_PAGE_SIZE);
	s->mmapBlockCount = 0;
	s->mmapBlockFirst = 0;
	s->mmapBlockSize = VGC_MALLOC_MMAP_PAGES * s->pageSize;

#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY)
	s->isMprotectEnabled = true;
#  if defined(VGC_MALLOC_MPROTECT_MP)
	s->maxProcesses = 0;
	s->children = 0;
#  endif
#else
	s->isMprotectEnabled = false;
#endif
	return s;
}


// initializeShared
//
static bool initializeShared(void)
{
	shared = createShared();
	if (!shared) return false;
	if (!initialiseMutex()) return false;

#if defined(VGC_MALLOC_MPROTECT_MP) && (defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_MPROTECT_PKEY))
	if (shared->isMprotectEnabled) startMprotect(10);
#endif
	return true;
}



// vgc_malloc
//
// The vgc_malloc() function allocates size bytes and returns a pointer to the allocated memory.
// The memory is not initialized.
// If size is 0, then vgc_malloc() returns NULL.
//
// Returns:
// The vgc_malloc() function returns a pointer to the allocated memory that is suitably aligned for any kind of variable.
// On error, this function returns NULL.
// NULL may also be returned by a successful call to vgc_malloc() with a size of zero.
//
ATTR_PUBLIC void *vgc_malloc(size_t size)
{
	if (shared->mmapBlockFirst == 0) {
		// Initialise first MMAP block
		//
		shared->mmapBlockFirst = mmapBlockAllocate(shared->mmapBlockSize, 0);
		if (shared->mmapBlockFirst == MAP_FAILED) return 0;
	}

	if (size == 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "size", "Warning",  "size is zero", 0);
		return 0;
	}

	if (!shared->isMprotectEnabled) {
		size += size % sizeof(char*) == 0 ? 0 : sizeof(char*) - (size % sizeof(char*));  // Align to 64bit, could use this? __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)))
	}

	if (size >= shared->mmapBlockFirst->maxSize) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "size", "Error", "size is too big", ": %d", size);
		return 0;
	}

	if (!PTHREAD_mutexLock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on shared mutex", 0);
		return 0;
	}

	VGC_mmapHeader *mmapBlockLast = 0;
	for (register VGC_mmapHeader *mmapBlock = shared->mmapBlockFirst; mmapBlock != 0; mmapBlock = mmapBlock->next) {
		void *memory = allocMallocBlock(mmapBlock, size);
		if (memory != 0) {
			if (!PTHREAD_mutexUnlock(&shared->mutex)) {
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
			}
			return memory;
		}
		mmapBlockLast = mmapBlock;
	}
#if 0
	dumpMmapBlock(0, 0, "memory", mmapBlockLast, "memory dump");
#endif

	// There was no more space in the allocated MMAP blocks
	// Allocate a new MMAP block and obtain new memory from it
	//
	VGC_mmapHeader *next = mmapBlockAllocate(shared->mmapBlockSize, mmapBlockLast);
	if (next == MAP_FAILED) {
		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		return 0;
	}

	void *memory = allocMallocBlock(next, size);

	if (!PTHREAD_mutexUnlock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
	}

	return memory;
}


// Deallocate the MMAP block if it is all free
//
static void freeMMAP(VGC_mmapHeader *mmapBlock)
{
	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unmapping MMAP at", "memory address", 0, "0x%lx (size: %dKB)", mmapBlock, mmapBlock->size / 1024);

	shared->mmapBlockCount--;
	if (mmapBlock == shared->mmapBlockFirst) shared->mmapBlockFirst = 0;
	if (munmap(mmapBlock, mmapBlock->size) == -1) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP", ": %s", strerror(errno));
	}
}


// Check if next blocks are free and unify
//
static void freeBlocksNext(VGC_mallocHeader *mallocBlock)
{
	VGC_mallocHeader *nextBlock = mallocBlock->next;
	if (nextBlock != 0 && nextBlock->status == VGC_MALLOC_FREE) {
		VGC_munprotect(nextBlock);
		mallocBlock->size += nextBlock->size + sizeof(VGC_mallocHeader);
		mallocBlock->next = nextBlock->next;
		if (nextBlock->next) nextBlock->next->prev = mallocBlock;
	}
}

 
// Check if previous blocks are free and unify
//
static void freeBlocksPrev(VGC_mallocHeader *mallocBlock)
{
	VGC_mallocHeader *prevBlock = mallocBlock->prev;
	if (prevBlock != 0 && prevBlock->status == VGC_MALLOC_FREE) {
		VGC_munprotect(mallocBlock);
		prevBlock->size += mallocBlock->size + sizeof(VGC_mallocHeader);
		prevBlock->next = mallocBlock->next;
		if (prevBlock->next) prevBlock->next->prev = prevBlock;
	}
}


// vgc_free
//
// The vgc_free() function frees the memory space pointed to by ptr, which must have been returned by a previous
// call to vgc_malloc(), vgc_calloc() or vgc_realloc().
// Otherwise, or if vgc_free(ptr) has already been called before, undefined behavior occurs.
// If ptr is NULL, no operation is performed.
//
// Returns:
// The vgc_free() function returns no value.
//
ATTR_PUBLIC void vgc_free(void *ptr)
{
	if (ptr == 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "ptr", "Warning", "variable points to zero", 0);
		return;
	}

	if (!PTHREAD_mutexLock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on shared mutex", 0);
		return;
	}

	if (!isInMmapBlock(ptr)) {
		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "isInMmapBlock", "Error", "points outside of MMAP at", ": 0x%lx", ptr);
		return;
	}

	if (shared->isMprotectEnabled) {
		unsigned long int mask = shared->pageSize - 1;
		ptr = (void*)((unsigned long int)ptr & ~mask);
	}

	VGC_mallocHeader *mallocBlock = (VGC_mallocHeader*)((char*)ptr - sizeof(VGC_mallocHeader));

	if (mallocBlock->checkStart != 0xAA || mallocBlock->checkEnd != 0xAA) {
		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Free", "", "", "%d bytes (at 0x%lx)", mallocBlock->size, ptr);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mallocBlock", "Error", "wrong checksum in", ": %s (at 0x%lx)", mallocBlock->checkStart != 0xAA ? "checkStart" : "checkEnd", ptr);
		dumpMmapBlock(0, 0, "vgc_free", mallocBlock->mmapBlock, "MMAP block wrong checksum");
		return;
	}

	VGC_mmapHeader *mmapBlock = mallocBlock->mmapBlock;
	if (mmapBlock->checkStart != 0xAA || mmapBlock->checkEnd != 0xAA) {
		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Free", "", "", "%d bytes (at 0x%lx)", mallocBlock->size, ptr);
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mmapBlock", "Error", "wrong checksum in", ": %s (at 0x%lx)", mmapBlock->checkStart != 0xAA ? "checkStart" : "checkEnd", mmapBlock);
		dumpMmapBlock(0, 0, "vgc_free", mmapBlock, "MMAP block wrong checksum");
		return;
	}

	mmapBlock->elements--;
	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Free", "", "", "%d bytes at 0x%lx (#%u)", mallocBlock->size, ptr, mmapBlock->elements);

	if (!PTHREAD_mutexLock(&mmapBlock->mutex)) {
		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on mmapBlock", ": %s", strerror(errno));
		return;
	}

	// Free this block and any free blocks close to it to make it a bigger unified free block
	//
	if (!checkMmapBlock("vgc_free", mmapBlock)) {
		if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
		}
		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "checkMmapBlock", "Error", "The MMAP for vgc_free is unstable while freeing memory", 0);
		return;
	}

	if (shared->isMprotectEnabled) {
		vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unprotect", "", "", "unprotect:0x%lx - free:0x%lx-0x%lx (%s) - size:%d - pid:%u", mallocBlock, ptr, (char*)ptr + mallocBlock->size, mallocBlock->status == VGC_MALLOC_FREE ? "free" : "busy", mallocBlock->size, getpid());
	}

	mallocBlock->status = VGC_MALLOC_FREE;
	VGC_munprotect(mallocBlock);
	freeBlocksNext(mallocBlock);
	freeBlocksPrev(mallocBlock);

	VGC_mallocHeader *first = firstMallocHeaderInMMAP(mmapBlock);
	if (first->next == 0) {
		// The allocated MMAP block is all free, it can be deallocated
		//
		if (mmapBlock->next != 0) {
			if (!PTHREAD_mutexLock(&mmapBlock->next->mutex)) {
				if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
					vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
				}
				if (!PTHREAD_mutexUnlock(&shared->mutex)) {
					vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
				}
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on mmapBlock->next", 0);
				return;
			}

			mmapBlock->next->prev = mmapBlock->prev;

			if (!PTHREAD_mutexUnlock(&mmapBlock->next->mutex)) {
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock->next mutex", 0);
			}
		}

		if (mmapBlock->prev != 0) {
			if (!PTHREAD_mutexLock(&mmapBlock->prev->mutex)) {
				if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
					vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
				}
				if (!PTHREAD_mutexUnlock(&shared->mutex)) {
					vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
				}
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on mmapBlock->prev", 0);
				return;
			}

			mmapBlock->prev->next = mmapBlock->next;

			if (!PTHREAD_mutexUnlock(&mmapBlock->prev->mutex)) {
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock->prev mutex", 0);
			}
		}

		if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
		}
		if (!PTHREAD_mutexDestroy(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexDestroy", "Error", "can't destroy mmapBlock mutex", 0);
		}
		if (!PTHREAD_mutexattrDestroy(&mmapBlock->mutexAttr)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexattrDestroy", "Error", "can't destroy attr mmapBlock mutex", 0);
		}

		freeMMAP(mmapBlock);

		if (!PTHREAD_mutexUnlock(&shared->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		}
		return;
	}

	if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
	}
	if (!PTHREAD_mutexUnlock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
	}
}


// vgc_calloc
//
// The vgc_calloc() function  allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory.
// The memory is set to zero.
// If nmemb or size is 0, then vgc_calloc() returns NULL.
//
// Returns:
// The vgc_calloc() function returns a pointer to the allocated memory that is suitably aligned for any kind of variable.
// On error, this function returns NULL.
// NULL may also be returned by a successful call to vgc_calloc() with nmemb or size equal to zero.
//
ATTR_PUBLIC void *vgc_calloc(size_t nmemb, size_t size)
{
	size_t mallocSize = nmemb * size;
	if (mallocSize == 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mallocSize", "Warning", "total size is zero", 0);
		return 0;
	}

	void *ptr = vgc_malloc(mallocSize);
	return ptr == 0 ? 0 : memset(ptr, 0, mallocSize);
}


// vgc_realloc
//
// The vgc_realloc() function changes the size of the memory block pointed to by ptr to size bytes.
// The contents will be unchanged in the range from the start of the region up to the minimum of the old and new sizes.
// If the new size is larger than the old size, the added memory will not be initialized.
// If ptr is NULL, then the call is equivalent to vgc_malloc(size), for all values of size;
// if size is equal to zero, and ptr is not NULL, then the call is equivalent to vgc_free(ptr).
// Unless ptr is NULL, it must have been returned by an earlier call to vgc_malloc(), vgc_calloc() or vgc_realloc().
// If the area pointed to was moved, a vgc_free(ptr) is done.
//
// Returns:
// The vgc_realloc() function returns a pointer to the newly allocated memory, which is suitably aligned for any kind of variable
// and may be different from ptr, or NULL if the request fails.
// If size was equal to 0, NULL is returned.
// If vgc_realloc() fails the original block is left untouched; it is not freed or moved.
//
ATTR_PUBLIC void *vgc_realloc(void *ptr, size_t size)
{
	// per una implementazione ottimale:
	//  vedere se il blocco successivo è libero in caso di size più grande della vecchia
	//  se size minore della vecchia il blocco viene diviso in due e il resto viene allocato come libero
	//  copiare la vecchia memoria nella nuova se non c'è posto contiguo
	//
	if (size == 0) {
		if (ptr != 0) {
			vgc_free(ptr);
		}
		return 0;
	}

	if (ptr == 0) return vgc_malloc(size);

	void *new = vgc_malloc(size);
	if (new == 0) return 0;

	VGC_mallocHeader *mallocBlock = (VGC_mallocHeader*)((char*)ptr - sizeof(VGC_mallocHeader));	// Get size of currently pointed block (by "ptr" parameter)
	size_t newSize = MIN(mallocBlock->size, size);	// Calculate the minimum of the current and old sizes
	memcpy(new, ptr, newSize);
	vgc_free(ptr);
	return new;
}


#ifdef VGC_MALLOC_DEBUG_MMAP
// vgc_mallocCheckMMAP
//
ATTR_PUBLIC void vgc_mallocCheckMMAP(char *response, int size)
{
	if (shared == 0) return;

	const char *str = "Debug memory";

	if (!PTHREAD_mutexLock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on shared mutex", 0);
	}
	for (register VGC_mmapHeader *mmapBlock = shared->mmapBlockFirst; mmapBlock != 0; mmapBlock = mmapBlock->next) {
		if (!PTHREAD_mutexLock(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on mmapBlock", 0);
		}
		if (!checkMmapBlock(str, mmapBlock)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, str, "Found memory problem", "Error", "MMAP memory allocation");
		}
		dumpMmapBlock(response, size, str, mmapBlock, "Dump MMAP memory allocation for vgc_malloc");
		if (!PTHREAD_mutexUnlock(&mmapBlock->mutex)) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock mmapBlock mutex", 0);
		}
	}
	if (!PTHREAD_mutexUnlock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
	}
}
#endif
#if 0
// vgc_mallocIsAllocated
//
ATTR_PUBLIC bool vgc_mallocIsAllocated(void *ptr)
{
	if (!PTHREAD_mutexLock(&shared->mutex)) {
		vgc_message(1, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexLock", "Error", "can't get lock on shared mutex", 0);
		return false;
	}

	bool ret = isInMmapBlock(ptr);

	if (!PTHREAD_mutexUnlock(&shared->mutex)) {
		vgc_message(1, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		return false;
	}

	return ret;
}


// vgc_mallocSizeMMAP
//
ATTR_PUBLIC int vgc_mallocSizeMMAP(void)
{
	return shared->mmapBlockSize;
}


// vgc_mallocCountMMAP
//
ATTR_PUBLIC int vgc_mallocCountMMAP(void)
{
	return shared->mmapBlockCount;
}


// vgc_mallocCloseService
//
// Close service module
//
ATTR_PUBLIC bool vgc_mallocCloseService(void)
{
	bool ret = true;

	if (!PTHREAD_mutexLock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't lock shared mutex", 0);
		ret = false;
	}
	if (!PTHREAD_mutexUnlock(&shared->mutex)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_mutexUnlock", "Error", "can't unlock shared mutex", 0);
		ret = false;
	}

	return ret;
}
#endif
