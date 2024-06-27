//
// Copyright (C) 2012-2024 by Vincenzo Capuano
//
#pragma once

#include <stdbool.h>
#include <stddef.h>


// Defines
//

// This is used mostly in downloading modules to make the function visible globally
//
#define ATTR_PUBLIC __attribute__((visibility("default")))

// This is used for unused parameters
//
#define ATTR_UNUSED __attribute__ ((__unused__))

// This is used by library module constructor
//
#define ATTR_CONSTRUCTOR __attribute__((constructor))

// This is used by library module destructor
//
#define ATTR_DESTRUCTOR __attribute__((destructor))
