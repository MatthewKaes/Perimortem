// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_TERMINAL_NATIVE_PROVIDER_H
#define TETRODOTOXIN_TERMINAL_NATIVE_PROVIDER_H

#include "ttx/abi.h"

// Native executable production begins only after Environment supplies one
// exact compiler-and-linker observation. The requirement names that contract
// without making compiler paths, archives, or link options part of TTX.
TTX_EXTERN_C TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_native_toolchain_requirement(void);

#endif
