//
// Copyright (C) 2012-2018 by Vincenzo Capuano
//
#pragma once


// Defines
//
typedef enum VGC_errorType {
	FE_initialise,
	FE_callstack,
	FE_configuration,
	FE_malloc,
	FE_unknownError
} VGC_errorType;


// Functions
//
void vgc_fatalErrorHandling(void);
void vgc_fatalError(VGC_errorType val);
void vgc_terminate(VGC_errorType val);

