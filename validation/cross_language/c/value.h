// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_CROSS_LANGUAGE_C_VALUE_H
#define VALIDATION_CROSS_LANGUAGE_C_VALUE_H

#include "cross_language/bridge.h"

ttx_abstract ttx_test_c_value_create(
    ttx_borrowed_bytes name,
    ttx_borrowed_bytes text,
    ttx_abstract value_requirement,
    ttx_abstract view_bytes_requirement,
    ttx_abstract to_string_operation);

#endif
