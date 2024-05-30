//
// Copyright (C) 2012-2024 by Vincenzo Capuano
//
#pragma once

#include <stdbool.h>


// Defines
//
#define INFO_LEVEL  0
#define ERROR_LEVEL 1
#define DEBUG_LEVEL 3

extern const char *const c_red;
extern const char *const c_green;
extern const char *const c_blue;
extern const char *const c_black;
extern const char *const DASHES;


// Functions
//
void vgc_message(const int level, char *file, int line, const char *moduleName, const char *functionName, const char *title, const char *subtitle, const char *status, const char *fmt, ...);
void vgc_messageInit(void);
