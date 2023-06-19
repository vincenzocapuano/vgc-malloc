//
// Copyright (C) 2016-2020 by Vincenzo Capuano
//
#define _GNU_SOURCE	// Needed for REG_RIP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#include <ucontext.h>

#include "vgc_message.h"
#include "vgc_stacktrace.h"


static const char *moduleName = "STACKTRACE";

#define SI_FROMUSER(siptr) (siptr)->si_code


#ifdef VGC_MALLOC_STACKTRACE
typedef struct ProcessPath {
	char *base;
	char  path[128];
} ProcessPath;

// Get process name and path
//
static ProcessPath *getProcessPath(void)
{
	static ProcessPath self;

	self.base = 0;
	memset(self.path, 0, sizeof(self.path));
	if (readlink("/proc/self/exe", self.path, sizeof(self.path) - 1) >= 0) {
		self.base = strrchr(self.path, '/');
		self.base++;
	}
	return &self;
}
#endif


void vgc_stacktraceShow(VGC_mallocHeader *mallocBlock)
{
#ifdef VGC_MALLOC_STACKTRACE
	if (mallocBlock->btArraySize == 0) return;

	char **messages = backtrace_symbols(mallocBlock->btArray, mallocBlock->btArraySize);
	if (messages == 0) return;

	ProcessPath *self = getProcessPath();

	for (int i = 2; i + 1 < mallocBlock->btArraySize; i++) {
		if (strncmp(messages[i], "/lib64/", strlen("/lib64/")) == 0) continue;
		if (strncmp(messages[i], "/usr/lib64/", strlen("/usr/lib64/")) == 0) continue;
		char *space = strchr(messages[i], '(');
		char *end = strchr(space, ')');
		*space = ' ';
		*end = ' ';
		end = strchr(end, '[');
		*end = ' ';
		end = strchr(end, ']');
		*end = 0;

		char addr2line[1024];
		snprintf(addr2line, 1024, "addr2line -Cfipe %s%s", strncmp(messages[i], self->base, strlen(self->base)) == 0 ? (*self->base = 0, self->path) : "", messages[i]);
		FILE *in = popen(addr2line, "r");
		if (in != 0) {
			char result[1024];
			fgets(result, 1024, in);
			pclose(in);
			*strchr(result, '\n') = 0; // Removed newline in addr2line string
			if (result[0] != '?') vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "[bt]", "    Stack trace in", 0, "%s", result);
		}
	}

	free(messages);
#endif
}


// This is used to intercept SIGxxx errors and print the stack trace
// to identify the caller that generated the error
// 
void vgc_stacktraceSave(VGC_mallocHeader *mallocBlock)
{
#ifdef VGC_MALLOC_STACKTRACE
	mallocBlock->btArraySize = backtrace(mallocBlock->btArray, VGC_MALLOC_STACKTRACE_SIZE);
#endif
}


#ifdef VGC_MALLOC_STACKTRACE_SIGNAL
static void continueAfterCrash(void)
{
	printf("resuming after crash: normal running now, closing nicely\n");

	// show the reason for the bug and exit

	exit(EXIT_FAILURE);
}


static void vgc_stacktrace(int sigNumber, siginfo_t *info, void *ucontext)
{
	struct sigaction action;

	switch(sigNumber) {
		case SIGSEGV:
			// Breaks if it is a memory bounds violation error
			//
			//
			if (SI_FROMUSER(info) == SEGV_ACCERR || SI_FROMUSER(info) == SEGV_BNDERR || SI_FROMUSER(info) == SEGV_PKUERR) break;

		default:
			// NOTE: This is needed to also trigger a coredump.
			//       Otherwise, as the signal was intercepted and processed in this function, there would not be a coredump
			//
			action.sa_flags = SA_SIGINFO;
			action.sa_handler = SIG_DFL;
			sigaction(sigNumber, &action, 0);
			kill(getpid(), sigNumber);
			return;
	}

	ucontext_t  *uc = ucontext;
	void        *btArray[VGC_MALLOC_STACKTRACE_SIZE];
	int          size = backtrace(btArray, VGC_MALLOC_STACKTRACE_SIZE);
	char       **messages = backtrace_symbols(btArray, size);

	// Get the address at the time the signal was raised
	// Only gcc is supported
	//
	void *callerAddress = (void*)uc->uc_mcontext.gregs[REG_RIP];
	btArray[1] = callerAddress;	// Overwrite sigaction with caller's address

	ProcessPath *self = getProcessPath();

	// Avoids the crash situation and continue
	//
	uc->uc_mcontext.gregs[REG_RIP] = (unsigned long int)continueAfterCrash;

	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "[bt]", 0, 0, "signal %d (%s), address is 0x%lx from 0x%lx", sigNumber, strsignal(sigNumber), info->si_addr, callerAddress);

	for (int i = 2; i < size - 1; i++) {
		if (strncmp(messages[i], "/lib64/", strlen("/lib64/")) == 0) continue;
		if (strncmp(messages[i], "/usr/lib64/", strlen("/usr/lib64/")) == 0) continue;

		char *space = strchr(messages[i], '(');
		char *end = strchr(space, ')');
		*space = ' ';
		*end = ' ';
		end = strchr(end, '[');
		*end = ' ';
		end = strchr(end, ']');
		*end = 0;

		char addr2line[1024];
		snprintf(addr2line, 1024, "addr2line -Cfipe %s%s", strncmp(messages[i], self->base, strlen(self->base)) == 0 ? (*self->base = 0, self->path) : "", messages[i]);
		FILE *in = popen(addr2line, "r");
		if (in != 0) {
			fgets(addr2line, 1024, in);
			pclose(in);
			if (addr2line[0] != '?') {
				int len = strlen(addr2line) - 1;
				if (len > 0 && addr2line[len] == '\n') {
					addr2line[len] = 0;
				}
				vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "[bt]", "Memory error in", 0, "%s", addr2line);
			}
		}
	}

	free(messages);
}
#endif


// vgc_staktraceInit
// SIGSEGV generated by mprotect'ed blocks (is it possible to know if it really comes from that? TODO
//
bool vgc_stacktraceInit(void)
{
#ifdef VGC_MALLOC_STACKTRACE_SIGNAL
	// Initialise signals management
	//
	int signals[] = { /*SIGQUIT, SIGILL, SIGABRT, SIGBUS, SIGFPE,*/ SIGSEGV, 0 };
	struct sigaction sigact;

	sigact.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_sigaction = vgc_stacktrace;

	// Block every signal during the handler
	//
	sigfillset(&sigact.sa_mask);

	// Handle signals
	//
	for (int sig = 0; signals[sig] != 0; sig++) {
		vgc_message(DEBUG_LEVEL, __FILE__, __LINE__, moduleName, __func__, "sigaction", "initialise handler for signal", 0, "(%d) %s", signals[sig], strsignal(signals[sig]));
		if (sigaction(signals[sig], &sigact, 0) != 0) {
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "sigaction", "Error", "initialising handler for signal", ": (%d) %s", signals[sig], strsignal(signals[sig]));
			return false;
		}
	}
#endif
	return true;
}
