//
// Copyright (C) 2015-2024 by Vincenzo Capuano
//

// The number of concurrently open mprotect() we can call is regulated by parameter /proc/sys/vm/max_map_count
//
#include "vgc_common.h"
#include "vgc_mprotect.h"
#include "vgc_mprotect_mp.h"

extern VGC_shared *shared;

#ifdef VGC_MALLOC_MPROTECT_PKEY

#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "vgc_message.h"
#include "vgc_network.h"


static const char *moduleName = "VGC-MALLOC-MPROTECT";



static bool do_mprotect(void *addr, size_t len, int prot, int *pkey)
{
#if 1
	if (prot == PROT_NONE) {
		*pkey = pkey_alloc(0, PKEY_DISABLE_ACCESS);
printf("-----> %d - errno: %d\n", *pkey, errno);
		if (*pkey == -1) {
			char *s = "protecting returned";
			switch(errno) {
				// pkey, flags, or access_rights is invalid
				//
				case EINVAL:

				// (pkey_alloc()) All protection keys available for the current process have been allocated.
				// The number of keys available is architecture-specific and implementation-specific and may
				// be reduced by kernel-internal use of certain keys.
				// ** There are currently 15 keys available to user programs on x86.**
				//
				// This  error will also be returned if the processor or operating system does not support protection keys.
				// Applications should always be prepared to handle this error, since factors out‐side of the application's
				// control can reduce the number of available pkeys.
				//
				case ENOSPC:
					vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", s, 0, "%d - %s", errno, strerror(errno));
					break;

				default:
					vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", s, 0, "%d - %s", errno, strerror(errno));
					break;
			}

			return false;
		}
	}

	int ret = pkey_mprotect(addr, len, prot, *pkey);
	if (ret == 0) {
		if (prot == PROT_NONE) return true;
		if (pkey_free(*pkey) == 0) {
			*pkey = 0;
			return true;
		}
	}

#else
	if (mprotect(addr, len, prot) == 0) return true;
#endif

	char *s = prot == PROT_NONE ? "protecting returned" : "unprotecting returned";

	switch(errno) {
		// The memory cannot be given the specified access. This can happen, for example, if you mmap(2) a file to which you have read-only access,
		// then ask mprotect() to mark it PROT_WRITE.
		//
		case EACCES:

		// 1. addr is not a valid pointer, or not a multiple of the system page size.
		// 2. (pkey_mprotect()) pkey has not been allocated with pkey_alloc(2)
		// 3. Both PROT_GROWSUP and PROT_GROWSDOWN were specified in prot.
		// 4. Invalid flags specified in prot.
		//
		case EINVAL:

		// 1. Internal kernel structures could not be allocated.
		// 2. Addresses in the range [addr, addr+len-1] are invalid for the address space of the process, or specify one or more pages that are not  mapped.
		// 3. Changing the protection of a memory region would result in the total number of mappings with distinct attributes (e.g., read versus read/write protection)
		//    exceeding the allowed maximum. (For example, making the protection of a range PROT_READ in the middle of a region currently protected as PROT_READ|PROT_WRITE
		//    would result in three mappings: two read/write mappings at each end and a read-only mapping in the middle.)
		//
		case ENOMEM:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", s, 0, "%d - %s", errno, strerror(errno));
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", s, 0, "%d - %s", errno, strerror(errno));
			break;
	}

	return false;
}


// VGC_mprotect
//
bool VGC_mprotect(VGC_mallocHeader *header)
{
//	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Protect", "", "", "protect:0x%lx - malloc:0x%lx-0x%lx (%s) - size:%d - pid:%u", header, (char*)header + sizeof(VGC_mallocHeader), (char*)header + sizeof(VGC_mallocHeader) + header->size, header->status == VGC_MALLOC_FREE ? "free" : "busy", header->size, getpid());

	const int prot = PROT_NONE;

	if (!do_mprotect(header->protect, shared->pageSize, prot)) return false;
#ifdef VGC_MALLOC_MPROTECT_MP
	mprotectDistribute(header, prot);
#endif
	return true;
}


// VGC_munprotect
//
bool VGC_munprotect(VGC_mallocHeader *header)
{
//	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unprotect", "", "", "unprotect:0x%lx - malloc:0x%lx-0x%lx (%s) - size:%d - pid:%u", header, (char*)header + sizeof(VGC_mallocHeader), (char*)header + sizeof(VGC_mallocHeader) + header->size, header->status == VGC_MALLOC_FREE ? "free" : "busy", header->size, getpid());

	const int prot = PROT_READ | PROT_WRITE;

	if (!do_mprotect(header->protect, shared->pageSize, prot)) return false;
#ifdef VGC_MALLOC_MPROTECT_MP
	mprotectDistribute(header, prot);
#endif
	return true;
}
#endif
