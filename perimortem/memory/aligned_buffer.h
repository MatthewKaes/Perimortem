// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_MEMORY_ALIGNED_BUFFER_H
#define PERIMORTEM_MEMORY_ALIGNED_BUFFER_H

#include "perimortem/core/bibliotheca.h"

// Aligned Buffer is the storage primitive beneath owner specific collections.
// It knows only byte capacity and alignment. Element count, construction,
// destruction, ordering, and relocation remain with the semantic owner that
// knows the real entry type.
//
// This differs from realloc and a C++ vector. Creating a replacement never
// copies unknown values or runs hidden element operations. A trivially
// relocatable owner may copy its bytes, while another owner can move each live
// entry before releasing the old buffer. Bibliotheca supplies stronger
// alignment than most entries require, but the request remains explicit so a
// future allocator cannot silently weaken an established ABI.
struct perimortem_aligned_buffer {
  uint8_t* data;
  perimortem_count capacity;
};

#ifdef __cplusplus
static_assert(
    sizeof(struct perimortem_aligned_buffer) == sizeof(uint8_t*) * 2,
    "Aligned Buffer has one pointer and one fixed width capacity");
static_assert(
    alignof(struct perimortem_aligned_buffer) == alignof(uint8_t*),
    "Aligned Buffer uses pointer alignment");
#else
_Static_assert(
    sizeof(struct perimortem_aligned_buffer) == sizeof(uint8_t*) * 2,
    "Aligned Buffer has one pointer and one fixed width capacity");
_Static_assert(
    _Alignof(struct perimortem_aligned_buffer) == _Alignof(uint8_t*),
    "Aligned Buffer uses pointer alignment");
#endif

PERIMORTEM_EXTERN_C perimortem_bool perimortem_aligned_buffer_create(
    perimortem_count required_bytes,
    perimortem_count alignment,
    struct perimortem_aligned_buffer* result);
PERIMORTEM_EXTERN_C void perimortem_aligned_buffer_release(
    struct perimortem_aligned_buffer* buffer);

#endif
