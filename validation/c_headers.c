// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// This translation unit keeps the public C boundary inside the ordinary test
// build. Including the headers together catches language and declaration
// conflicts without maintaining a second validation executable or a parallel
// dependency graph.

#include "perimortem/core/bibliotheca.h"
#include "perimortem/core/diagnostics/fatal.h"
#include "perimortem/core/hash.h"
#include "perimortem/core/implementation.h"
#include "perimortem/core/object.h"
#include "perimortem/core/perimortem.h"

#include "perimortem/memory/aligned_buffer.h"
#include "perimortem/memory/buffer.h"
#include "perimortem/memory/hash_index.h"

#include "perimortem/system/terminal.h"

#include "tetrodotoxin/language/error.h"
#include "tetrodotoxin/language/production.h"
#include "tetrodotoxin/language/provider.h"
#include "tetrodotoxin/plugin/abi.h"
#include "tetrodotoxin/terminal/provider.h"
#include "ttx/abi.h"

void validation_c_headers_compile_together(void) {}
