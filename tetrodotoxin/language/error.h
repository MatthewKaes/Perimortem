// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_ERROR_H
#define TETRODOTOXIN_LANGUAGE_ERROR_H

#include "ttx/abi.h"

// A completed operational failure remains an exact identity owned by the
// subsystem that understood the request. This marker lets a caller distinguish
// that outcome from Unknown and None without imposing one error enum, message
// format, exception model, or value representation on participating languages.
TTX_EXTERN_C TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_error_requirement(void);

#endif
