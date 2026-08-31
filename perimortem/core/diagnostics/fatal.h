// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_DIAGNOSTICS_FATAL_H
#define PERIMORTEM_CORE_DIAGNOSTICS_FATAL_H

#include "perimortem/core/perimortem.h"

// Fatal is the allocation independent process failure path for load bearing
// Core systems. It writes one complete C string to the standard error stream
// and aborts without consulting graph, worker, file, or allocator state. Rich
// Diagnostics remain appropriate after those owners are known to be usable.
PERIMORTEM_EXTERN_C void perimortem_fatal(const char* message);

#endif
