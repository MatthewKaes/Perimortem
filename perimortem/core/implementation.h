// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_IMPLEMENTATION_H
#define PERIMORTEM_CORE_IMPLEMENTATION_H

#include "perimortem/core/object.h"

struct perimortem_projection;

// Implementation is the two word runtime carrier for one explicitly erased
// Object. The Object remains the owned identity. The projection is immutable
// Terminal data that describes how one accepted semantic Interface is exposed
// to a target. It is typed as an opaque projection rather than erased context
// because callers may not reinterpret it without already knowing the exact
// Interface contract that selected it.
//
// C makes every ownership transition explicit. A retained or copied carrier
// owns one Object reservation, an adopted carrier receives one reservation
// from its producer, and release clears both words after returning that
// reservation. Interface proof and projection construction remain outside
// this physical carrier.
struct perimortem_implementation {
  uint8_t* object;
  const struct perimortem_projection* projection;
};

#ifdef __cplusplus
static_assert(
    sizeof(struct perimortem_implementation) == sizeof(uint8_t*) * 2,
    "Implementation has one Object and one projection pointer");
static_assert(
    alignof(struct perimortem_implementation) == alignof(uint8_t*),
    "Implementation uses pointer alignment");
#else
_Static_assert(
    sizeof(struct perimortem_implementation) == sizeof(uint8_t*) * 2,
    "Implementation has one Object and one projection pointer");
_Static_assert(
    _Alignof(struct perimortem_implementation) == _Alignof(uint8_t*),
    "Implementation uses pointer alignment");
#endif

PERIMORTEM_EXTERN_C perimortem_bool perimortem_core_implementation_retain(
    uint8_t* object,
    const struct perimortem_projection* projection,
    struct perimortem_implementation* result);
PERIMORTEM_EXTERN_C void perimortem_core_implementation_adopt(
    uint8_t* object,
    const struct perimortem_projection* projection,
    struct perimortem_implementation* result);
PERIMORTEM_EXTERN_C void perimortem_core_implementation_copy(
    const struct perimortem_implementation* source,
    struct perimortem_implementation* result);
PERIMORTEM_EXTERN_C void perimortem_core_implementation_assign(
    struct perimortem_implementation* target,
    const struct perimortem_implementation* source);
PERIMORTEM_EXTERN_C void perimortem_core_implementation_move(
    struct perimortem_implementation* target,
    struct perimortem_implementation* source);
PERIMORTEM_EXTERN_C void perimortem_core_implementation_release(
    struct perimortem_implementation* implementation);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_core_implementation_is_empty(
    const struct perimortem_implementation* implementation);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_core_implementation_is_valid(
    const struct perimortem_implementation* implementation);

#endif
