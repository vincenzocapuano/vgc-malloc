//
// Copyright (C) 2012-2020 by Vincenzo Capuano
//
#include <setjmp.h>
#include <stdlib.h>

#include "vgc_common.h"
#include "vgc_message.h"
#include "vgc_error.h"


// Used by OB_fatalErrorHandling
//
static sigjmp_buf jumpOnFatalError;


ATTR_PUBLIC void vgc_fatalErrorHandling(void)
{
	int val = sigsetjmp(jumpOnFatalError, true);
	switch(val) {
		case FE_initialise:
			break;

		default:
			vgc_terminate(val);
			break;
	}
}


ATTR_PUBLIC void vgc_fatalError(VGC_errorType val)
{
	siglongjmp(jumpOnFatalError, val);
}


ATTR_PUBLIC void vgc_terminate(VGC_errorType val)
{
	switch(val) {
		case FE_callstack:
			break;

		case FE_malloc:
			break;

		case FE_unknownError:
		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, "MASTER", __func__, "vgc_fatalErrorHandling", "Terminating on Fatal Error", "with unknown code", ": %d", val);
			break;
	}

	exit(EXIT_FAILURE);
}

