// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_MEMORY_BUFFER_H
#define PERIMORTEM_MEMORY_BUFFER_H

#include "perimortem/core/object.h"

// Buffer extends one Object allocation with size class growth and copying.
// The Object remains the lifetime identity, while each Bytes, Image, Vector, or
// other semantic owner retains its own logical size and typed element policy.
// Growing one handle returns a replacement and releases only that handle, so
// aliases continue to observe the original fixed allocation.
PERIMORTEM_EXTERN_C uint8_t* perimortem_core_object_clone(
    uint8_t* payload,
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count element_size);
PERIMORTEM_EXTERN_C uint8_t* perimortem_core_object_reserve(
    uint8_t* payload,
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size,
    const uint8_t* default_value);

#endif
