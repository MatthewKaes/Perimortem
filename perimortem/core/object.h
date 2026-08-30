// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_OBJECT_H
#define PERIMORTEM_CORE_OBJECT_H

#include "perimortem/core/perimortem.h"

// Object is the nonnull one word carrier shared by the runtime and generated
// code. The descriptor is immutable for the lifetime of every allocation that
// names it. Finalization receives the payload before its storage is remitted.
struct perimortem_object_descriptor {
  perimortem_count size;
  perimortem_count alignment;
  void (*finalize)(uint8_t* payload);
};

PERIMORTEM_EXTERN_C uint8_t* perimortem_core_object_allocate(
    const struct perimortem_object_descriptor* descriptor);
PERIMORTEM_EXTERN_C uint8_t* perimortem_core_object_allocate_buffer(
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size);
PERIMORTEM_EXTERN_C void perimortem_core_object_retain(uint8_t* payload);
PERIMORTEM_EXTERN_C void perimortem_core_object_release(uint8_t* payload);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_core_object_capacity(uint8_t* payload);
PERIMORTEM_EXTERN_C uint8_t* perimortem_core_object_clone(
    uint8_t* payload,
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count element_size);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_core_object_reservations(uint8_t* payload);
PERIMORTEM_EXTERN_C uint8_t* perimortem_core_object_reserve(
    uint8_t* payload,
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size,
    const uint8_t* default_value);
PERIMORTEM_EXTERN_C void perimortem_core_object_finalize_trivial(
    uint8_t* payload);

#endif
