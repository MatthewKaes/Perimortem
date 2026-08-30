// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_PERIMORTEM_H
#define PERIMORTEM_CORE_PERIMORTEM_H

#include <stdint.h>

#ifdef __cplusplus
#define PERIMORTEM_EXTERN_C extern "C"
#else
#define PERIMORTEM_EXTERN_C
#endif

typedef uint8_t perimortem_bool;
typedef uint64_t perimortem_count;

enum {
  PERIMORTEM_FALSE = 0,
  PERIMORTEM_TRUE = 1,
};

struct perimortem_bytes {
  const uint8_t* data;
  perimortem_count size;
};

static inline perimortem_bool perimortem_bytes_equal(
    struct perimortem_bytes left,
    struct perimortem_bytes right) {
  perimortem_count index;
  if (left.size != right.size) {
    return PERIMORTEM_FALSE;
  }
  for (index = 0; index < left.size; ++index) {
    if (left.data[index] != right.data[index]) {
      return PERIMORTEM_FALSE;
    }
  }
  return PERIMORTEM_TRUE;
}

#endif
