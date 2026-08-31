// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_OBJECT_H
#define PERIMORTEM_CORE_OBJECT_H

#include "perimortem/core/perimortem.h"

// Object is the nonnull one word carrier shared by the runtime and generated
// code. A null carrier represents absence and is never an allocated Object.
// The descriptor is immutable for the lifetime of every allocation that names
// it. Finalization receives the payload before its storage is remitted.
//
// Object does not retain or release implicitly. Generated code and each native
// semantic owner place those operations at their value boundaries. This keeps
// the C carrier direct and makes ownership visible, at the cost of requiring
// every copied handle to state whether it is borrowed or retained.
struct perimortem_object_descriptor {
  perimortem_count size;
  perimortem_count alignment;
  void (*finalize)(uint8_t* payload);
};

#ifdef __cplusplus
static_assert(
    sizeof(struct perimortem_object_descriptor) ==
        sizeof(perimortem_count) * 2 + sizeof(void (*)(uint8_t*)),
    "Object descriptors have two counts followed by one finalizer");
static_assert(
    alignof(struct perimortem_object_descriptor) == alignof(perimortem_count),
    "Object descriptors use the count alignment");
#else
_Static_assert(
    sizeof(struct perimortem_object_descriptor) ==
        sizeof(perimortem_count) * 2 + sizeof(void (*)(uint8_t*)),
    "Object descriptors have two counts followed by one finalizer");
_Static_assert(
    _Alignof(struct perimortem_object_descriptor) == _Alignof(perimortem_count),
    "Object descriptors use the count alignment");
#endif

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
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_core_object_reservations(uint8_t* payload);
PERIMORTEM_EXTERN_C const struct perimortem_object_descriptor*
    perimortem_core_object_descriptor(uint8_t* payload);
PERIMORTEM_EXTERN_C void perimortem_core_object_finalize_trivial(
    uint8_t* payload);

#endif
