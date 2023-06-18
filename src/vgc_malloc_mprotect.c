//
// Copyright (C) 2015-2020 by Vincenzo Capuano
//
#include "vgc_common.h"
#include "vgc_malloc_mprotect.h"

extern VGC_shared *shared;

#ifdef VGC_MALLOC_MPROTECT

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
static const char *SocketPath = "/tmp/vgc-malloc";
static const char *SocketName = "%s/malloc-%d.sock";


typedef struct MProtectBlock {
	VGC_mallocHeader *header;
	int               prot;
	pid_t             sourcePID;
} MProtectBlock;


// findFreeChild
//
static int findFreeChild(pid_t pid)
{
	for (int pos = 0; pos < shared->maxProcesses; pos++) {
		if (shared->children[pos].pid == pid) return -1;
	}

	for (int pos = 0; pos < shared->maxProcesses; pos++) {
		if (shared->children[pos].pid == 0) {
			shared->children[pos].pid = pid;
			return pos;
		}
	}

	return -1;
}


// countChildren
//
static int countChildren(void)
{
	int count = 0;

	for (int pos = 0; pos < shared->maxProcesses; pos++) {
		if (shared->children[pos].pid != 0) count++;
	}

	return count;
}


static bool do_mprotect(void *addr, size_t len, int prot)
{
	if (mprotect(addr, len, prot) == 0) return true;

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


// mprotectDistribute
//
// manageCorruptionThread
//
static void *manageCorruptionThread(void *_child)
{
	VGC_mallocDebugChild *child = _child;

	// Create Unix domain socket
	//
	snprintf(child->socketName, SocketNameSize, SocketName, SocketPath, child->pid);

	child->socketFD = socket(AF_UNIX, SOCK_STREAM, 0);
	if (child->socketFD == -1) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "socket", 0, "%d - %s", errno, strerror(errno));
		return 0;
	}

	struct sockaddr_un server;
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, child->socketName, sizeof(server.sun_path));

	if (!do_bind(child->socketFD, (struct sockaddr*)&server, sizeof(struct sockaddr_un), child->socketName)) return 0;
 	if (!do_listen(child->socketFD, 5)) return 0;

	MProtectBlock block;
	int fd;
	while(true) {
		if (!do_accept(child->socketFD, 0, 0, &fd)) break;

		size_t readBytes;
		while(true) {
			if (!do_read(fd, &block, sizeof(MProtectBlock), &readBytes)) {
				close(fd);
				break;
			}

			if (readBytes == 0) continue;
			if (child->pid == block.sourcePID) break;

			vgc_message(VGC_MALLOC_DEBUG_LEVEL + 1, __FILE__, __LINE__, moduleName, __func__, "Debug corruption thread", "", "", "%sprotect at 0x%lx - pid %u from pid %u", block.prot == PROT_NONE ? "" : "un", block.header->protect, child->pid, block.sourcePID);

			if (!do_mprotect(block.header->protect, shared->pageSize, block.prot)) {
				// check errno, error
				//
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "", "", "%sprotecting returned: %d - %s - at 0x%lx - pid %u from pid %u", block.prot == PROT_NONE ? "" : "un", errno, strerror(errno), block.header->protect, child->pid, block.sourcePID);
				if (block.prot != PROT_NONE) {
					// In case of unprotect
					//
					int result = 0;
					do_write(fd, &result, sizeof(int));
				}
				break;
			}

			int result = 1;
			if (!do_write(fd, &result, sizeof(int))) {
				close(fd);
				break;
			}
		}

		close(fd);
	}

	return 0;
}


// startChildThread
//
static bool startChildThread(VGC_mallocDebugChild *child)
{
	vgc_message(VGC_MALLOC_DEBUG_LEVEL + 2, __FILE__, __LINE__, moduleName, __func__, "Start thread", "", "", "Thread pid: %d", child->pid);

	// Start thread
	//
	if (!PTHREAD_create(&child->thread, 0, manageCorruptionThread, child)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_create", "Error", "thread create failed", 0);
		return false;
	}
	return true;
}


// removeChildThread
//
static void removeChildThread(VGC_mallocDebugChild *child)
{
	vgc_message(VGC_MALLOC_DEBUG_LEVEL + 2, __FILE__, __LINE__, moduleName, __func__, "Stop thread", "", "", "Thread pid: %d", child->pid);

	// Stop thread
	//
	if (!PTHREAD_cancel(child->thread)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_cancel", "Error", "cancelling thread", 0);
	}
	if (!PTHREAD_join(child->thread, 0)) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "PTHREAD_join", "Error", "joining thread", 0);
	}

	child->pid = 0;
	child->thread = 0;
	close(child->socketFD);
	unlink(child->socketName);
}


// startChildDebugCorruption
//
static void startChildDebugCorruption(ATTR_UNUSED const char *name, ATTR_UNUSED int type, void *_shared)
{
	VGC_shared *s = _shared;

	int pos = findFreeChild(getpid());
	if (pos == -1) return;
	VGC_mallocDebugChild *child = &s->children[pos];
	if (!startChildThread(child)) return;

	// Special case to manage the father
	//
	int count = countChildren();
	if (count > 1 && !s->isFather) {
		s->isFather = true;
		int p = findFreeChild(getpid());
		if (p == -1) return;
		VGC_mallocDebugChild *father = &s->children[p];
		if (!startChildThread(father)) return;
	}
}


static void mprotectDistribute(VGC_mallocHeader *header, int prot)
{
	MProtectBlock block;

	block.header    = header;
	block.prot      = prot;
	block.sourcePID = getpid();

	// send to all children
	//
	for (int i = 0; i < shared->maxProcesses; i++) {
		VGC_mallocDebugChild *child = &shared->children[i];
		if (child->pid == 0) continue;
		if (child->pid == block.sourcePID) continue;

		vgc_message(VGC_MALLOC_DEBUG_LEVEL + 2, __FILE__, __LINE__, moduleName, __func__, "Distribute", "", "", "%sprotect at 0x%lx - from pid %u to pid %u", prot == PROT_NONE ? "" : "un", block.header->protect, block.sourcePID, child->pid);

		int fd;
		if (!do_socket(AF_UNIX, SOCK_STREAM, 0, &fd)) break;

		struct sockaddr_un server;
		server.sun_family = AF_UNIX;
		strncpy(server.sun_path, child->socketName, sizeof(server.sun_path));

		if (!do_connect(fd, (struct sockaddr*)&server, sizeof(struct sockaddr_un))) {
			close(fd);
			removeChildThread(child);	// Remove this child from the list - it is probably dead due to a crash
			break;
		}

		if (!do_write(fd, &block, sizeof(MProtectBlock))) {
			close(fd);
			removeChildThread(child);	// Remove this child from the list - it is probably dead due to a crash
			break;
		}

		int answer;
		size_t readBytes;
		if (!do_read(fd, &answer, sizeof(int), &readBytes)) {
			close(fd);
			removeChildThread(child);	// Remove this child from the list - it is probably dead due to a crash
			break;
		}

//		if (readBytes == 0) continue;

		close(fd);
	}
}


// initChildDebugCorruption
//
static void initChildDebugCorruption(void)
{
	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Init debug corruption", "init", "done", 0);
	for (int i = 0; i < shared->maxProcesses; i++) {
		shared->children[i].pid = 0;
		shared->children[i].thread = 0;
	}

	shared->isFather = false;

	// Create sockets directory if does not exist
	//
	int m = mkdir(SocketPath, 0750);
	if (m == -1 && errno != EEXIST) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "mkdir", 0, "%d - %s", errno, strerror(errno));
		return;
	}

	// Check for old files in the directory, if any remove them
	//
	if (errno == EEXIST) {
		vgc_message(4, __FILE__, __LINE__, moduleName, __func__, "Warning", "mkdir", "directory exists - cleaning", 0);

		DIR *d = opendir(SocketPath);
		if (d) {
			for (struct dirent *dir = readdir(d); dir != 0; dir = readdir(d)) {
				if (dir->d_type == DT_SOCK) {
					char socketFile[1024];
					snprintf(socketFile, 1024, "%s/%s", SocketPath, dir->d_name);

					vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Removing socket file", 0, socketFile, 0);

					int u = unlink(socketFile);
					if (u == -1) {
						vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "Removing socket file", 0, "%s reason %s", socketFile, strerror(errno));
					}
				}
			}
			closedir(d);
		}
	}

#if 0
	struct passwd *passwd = getpwuid(geteuid());
	if (passwd == 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "getpwnam", 0, "%d - %s", errno, strerror(errno));
		return;
	}

	int c = chown(SocketPath, 0, passwd->pw_uid);
	if (c == -1) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "chown", 0, "%d - %s", errno, strerror(errno));
		return;
	}
#endif
}


// stopChildDebugCorruption
//
void stopChildDebugCorruption(ATTR_UNUSED const char *name, ATTR_UNUSED int type, void *_shared)
{
	VGC_shared *s = _shared;
	pid_t pid = getpid();

	for (int i = 0; i < shared->maxProcesses; i++) {
		VGC_mallocDebugChild *child = &s->children[i];
		if (child->pid == pid) {
			removeChildThread(child);
			break;
		}
	}
}


// VGC_mprotect
//
bool VGC_mprotect(VGC_mallocHeader *header)
{
//	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Protect", "", "", "protect:0x%lx - malloc:0x%lx-0x%lx (%s) - size:%d - pid:%u", header, (char*)header + sizeof(VGC_mallocHeader), (char*)header + sizeof(VGC_mallocHeader) + header->size, header->status == VGC_MALLOC_FREE ? "free" : "busy", header->size, getpid());

	const int prot = PROT_NONE;

	if (!do_mprotect(header->protect, shared->pageSize, prot)) return false;
	mprotectDistribute(header, prot);
	return true;
}


// VGC_munprotect
//
bool VGC_munprotect(VGC_mallocHeader *header)
{
//	vgc_message(VGC_MALLOC_DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unprotect", "", "", "unprotect:0x%lx - malloc:0x%lx-0x%lx (%s) - size:%d - pid:%u", header, (char*)header + sizeof(VGC_mallocHeader), (char*)header + sizeof(VGC_mallocHeader) + header->size, header->status == VGC_MALLOC_FREE ? "free" : "busy", header->size, getpid());

	const int prot = PROT_READ | PROT_WRITE;

	if (!do_mprotect(header->protect, shared->pageSize, prot)) return false;
	mprotectDistribute(header, prot);
	return true;
}
#endif


// startMprotect
//
bool startMprotect(int maxProcesses)
{
#ifdef VGC_MALLOC_MPROTECT
	shared->maxProcesses = maxProcesses;

	// Allocate space for children
	//
	shared->children = mmap(0, maxProcesses * sizeof(VGC_mallocDebugChild), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared->children == 0) {
		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "mmap", "Fatal error", "can't create shared MMAP memory for VGC_shared", ": %s", strerror(errno));
		return false;
	}

	initChildDebugCorruption();
	startChildDebugCorruption(0, 0, shared);

#ifdef C_ICAP
	// Run startChildDebugCorruption() on new child(process) started by c-icap
	//
	register_command_extend("Start MMAP boundary control", CHILD_START_CMD, shared, startChildDebugCorruption);

	// Run stopChildDebugCorruption() on child(process) stopped by c-icap
	//
	register_command_extend("Stop MMAP boundary control", CHILD_STOP_CMD, shared, stopChildDebugCorruption);
#endif
#endif
	return true;
}


// stopMprotect
//
void stopMprotect(void)
{
#ifdef VGC_MALLOC_MPROTECT
	// For master process
	//
	vgc_message(VGC_MALLOC_DEBUG_LEVEL + 2, __FILE__, __LINE__, moduleName, __func__, "Stopping master", "Stopping threads", "", 0);
	for (int i = 0; i < shared->maxProcesses; i++) {
		VGC_mallocDebugChild *child = &shared->children[i];
		if (child->pid != 0) {
			if (child->pid != getpid()) {
				removeChildThread(child);
			}
			else {
				// Master process
				//
				child->pid = 0;
				child->thread = 0;
				close(child->socketFD);
				unlink(child->socketName);
			}
		}
	}

	if (shared->children != 0) {
		if (munmap(shared->children, shared->maxProcesses * sizeof(VGC_mallocDebugChild)) == -1) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "munmap", "Error", "unmapping MMAP (shared->children)", ": %s", strerror(errno));
		}
	}
#endif
}


// createMprotect
//
void configMprotect(VGC_shared *s)
{
#ifdef VGC_MALLOC_MPROTECT
        s->isMprotectEnabled = true;
        s->maxProcesses = 0;
        s->children = 0;
#else
        s->isMprotectEnabled = false;
#endif
}


// VGC_mallocStatusDebugCorruption
//
// Display current configuration of debug memory corruption
//
ATTR_PUBLIC const char *VGC_mallocStatusDebugCorruption(void)
{
	if (shared->isMprotectEnabled) return "enabled";
	return "disabled";
}


// Start at each fork
//
void startChildMprotect(void)
{
#ifdef VGC_MALLOC_MPROTECT
	startChildDebugCorruption(0, 0, shared);
#endif
}


// Stop at the end of each child
//
void stopChildMprotect(void)
{
#ifdef VGC_MALLOC_MPROTECT
	stopChildDebugCorruption(0, 0, shared);
#endif
}
