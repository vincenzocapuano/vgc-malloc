//
// Copyright (C) 2012-2020 by Vincenzo Capuano
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "vgc_common.h"
#include "vgc_pthread.h"
#include "vgc_message.h"

#if 0
ATTR_PUBLIC extern int CI_DEBUG_STDOUT;
ATTR_PUBLIC extern void (*__log_error)(void *req, const char *format, ...);
#endif

typedef enum Type { TYPE_FUNCTION, TYPE_TITLE, TYPE_SUBTITLE, TYPE_ALL } Type;

static pid_t pid;
static int   messageLevel;

ATTR_PUBLIC const char *const c_black = "\033[0m";
ATTR_PUBLIC const char *const c_red   = "\033[0;31m";
ATTR_PUBLIC const char *const c_green = "\033[0;32m";
ATTR_PUBLIC const char *const c_blue  = "\033[0;34m";
ATTR_PUBLIC const char *const DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------";




ATTR_PUBLIC void vgc_messageInit(void)
{
	pid = getpid();
	messageLevel = 1;

	char *level = getenv("VGC_MESSAGE_LEVEL");
	if (level != 0) {
		long int l = strtol(level, 0, 10);
		errno = 0;
		if (errno == 0) messageLevel = l;
	}

//	CI_DEBUG_STDOUT = 0;
//	if (!isBackground) CI_DEBUG_STDOUT = 1;
}



static const char *dots(const char *string, Type type, const char *status)
{
	static const char *const d[TYPE_ALL] = {
		"......................: ",		// title
		"............................: ",	// subtitle
		".............................: "	// status
	};

	if (type == TYPE_SUBTITLE && status != 0 && status[0] == 0) return "";
	const int length = strlen(string) >= strlen(d[type]) ? strlen(d[type]) - 2 : strlen(string);
	return &d[type][length];
}


static const char *spaces(const char *string)
{
	static const char s[]          = "             ";
	static const int  spacesLength = sizeof(s) - 1;
	static const unsigned int MaxStringLength = 12;

	if (!string) return s;
	const int stringLength = strlen(string) > MaxStringLength ? MaxStringLength : strlen(string);
	const int length = stringLength > spacesLength ? spacesLength : stringLength;
	return &s[length];
}


static const char *spaces2(const char *string)
{
	static const char *const s = "........................";

	if (!string) return s;
	const int spacesLength = strlen(s);
	const int stringLength = strlen(string);
	int length = stringLength > spacesLength ? spacesLength : stringLength;
	return &s[length];
}


static int message(char messageBuf[], const int bufferSize, char *file, int line, const char *moduleName, const char *functionName, const char *title, const char *subtitle, const char *status, const char *color1, const char *color2, const char *color3)
{
	return snprintf(messageBuf, bufferSize, "%s%.30s:%d%s%s: %.22s%s%.28s%s%s%s%.12s%s%s%s%s%s%s%s",
			color1, basename(file), line, spaces2(basename(file)), line < 10 ? "..." : line < 100 ? ".." : line < 1000 ? "." : "",
			functionName != 0 ? functionName : "", functionName != 0 ? dots(functionName, TYPE_FUNCTION, status) : "",
			title != 0 ? title : "", title != 0 ? dots(title, TYPE_TITLE, status) : "",
			moduleName[0] ? color2 : "", moduleName[0] ? "[" : "", moduleName, moduleName[0] ? color2 : "", moduleName[0] ? "]" : "",
			moduleName[0] ? spaces(moduleName) : "", color3,
			subtitle != 0 ? subtitle : "", subtitle != 0 ? dots(subtitle, TYPE_SUBTITLE, status) : "",
			status != 0 ? status : "");
}


// If status == "" subtitle is put without dots
// If status == 0 subtitle is put with dots
// 	this allows to write values in the fmt, ... field(s) that are indented correctly
//
ATTR_PUBLIC void vgc_message(const int level, char *file, int line, const char *moduleName, const char *functionName, const char *title, const char *subtitle, const char *status, const char *fmt, ...)
{
	static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

	const int BufferSize = 2048;
	char      messageBuf[BufferSize];


	if (messageLevel < level) return;
//	if (__log_error == 0 && CI_DEBUG_STDOUT == 0) return;

	// Level =  0 - prints all in green
	// Level =  1 - prints all in red
	// Level >= 2 - prints default colors
	//
	const char *color1 = level == 1 ? c_red : level == 0 ? c_green : c_black;
	const char *color2 = level == 1 ? c_red : c_green;
	const char *color3 = level == 1 ? c_red : level == 0 ? c_green : c_blue;

#if 0
	// Logs on file
	//
	if (__log_error != 0) {
		int length = message(messageBuf, BufferSize, file, line, moduleName, functionName, title, subtitle, status, "", "", "");
		if (length < BufferSize && fmt != 0) {
			va_list myargs;
			va_start(myargs, fmt);
				vsnprintf(&messageBuf[length], BufferSize - length, fmt, myargs);
			va_end(myargs);
		}

		PTHREAD_rwlockRdLock(&rwlock);
			(*__log_error)(0, "%s\n", messageBuf);		// value originally from c-icap library
		PTHREAD_rwlockUnlock(&rwlock);
	}
#endif

//	if (CI_DEBUG_STDOUT != 0) {
		// Print on screen
		//
		int length = message(messageBuf, BufferSize, file, line, moduleName, functionName, title, subtitle, status, color1, color2, color3);
		if (length < BufferSize && fmt != 0) {
			va_list myargs;
			va_start(myargs, fmt);
				vsnprintf(&messageBuf[length], BufferSize - length, fmt, myargs);
			va_end(myargs);
		}

		PTHREAD_rwlockRdLock(&rwlock);
			printf("%s%s\n", messageBuf, c_black);		// value originally from c-icap library
		PTHREAD_rwlockUnlock(&rwlock);
//	}
}

#if 0
// Used by programs that do not link C-ICAP
//
static char G_logfile[256];

ATTR_PUBLIC void vgc_messageSetLogServer(const char *logfile)
{
	strncpy(G_logfile, logfile, 256);
#if 0
	struct stat sb;

	char *a = strrchr(G_logfile, '/');
	*a = 0;
	bool isDir = stat(G_logfile, &sb) == 0 && S_ISDIR(sb.st_mode);
	*a = '/';

	if (!isDir) {
		printf("Directory not found for log file: %s\n", logfile);
		G_logfile[0] = 0;
		return;
	}
#else
	if (!COMMON_checkDirectory(logfile)) {
		printf("Directory not found for log file: %s\n", logfile);
		G_logfile[0] = 0;
		return;
	}
#endif
}


ATTR_PUBLIC void vgc_messageLogServer(ATTR_UNUSED void *req, const char *format, ...)
{
	static FILE *logfile = (FILE*)-1;

	const int BufSize = 128;
	char      buf[BufSize];
	struct tm brTm;
	time_t    tm;


	if (logfile == (FILE*)-1) {
		logfile = stdout;
		if (G_logfile[0] != 0) {
			// open the file
			//
			logfile = fopen(G_logfile, "a+");
			if (logfile == 0) printf("Cannot open logfile: %s\n", G_logfile);
			if (logfile != 0) setvbuf(logfile, 0, _IONBF, 0);
		}
	}

	if (logfile == 0) return;

	time(&tm);
	asctime_r(localtime_r(&tm, &brTm), buf);
	buf[strlen(buf) - 1] = '\0';

	if (pid == getpid()) {
		strcat(buf, ", main proc");
	}
	else {
		snprintf(&buf[strlen(buf)], 100, ", %u/%u", pid, (unsigned int)PTHREAD_self());
	}

	va_list ap;
	va_start(ap, format);
		fprintf(logfile, "%s: ", buf);
		vfprintf(logfile, format, ap);
	va_end(ap);
}
#endif
