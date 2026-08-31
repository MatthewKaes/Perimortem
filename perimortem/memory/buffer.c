// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/buffer.h"

#include <string.h>

#include "perimortem/core/diagnostics/fatal.h"

static void validate_buffer(
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size,
    const uint8_t* default_value) {
  if (descriptor == 0 || element_size == 0 ||
      count > UINT64_MAX / element_size || (count != 0 && default_value == 0)) {
    perimortem_fatal("Core Object reserve received an invalid buffer contract");
  }
}

uint8_t* perimortem_core_object_clone(
    uint8_t* payload,
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count element_size) {
  if (descriptor == 0 || element_size == 0) {
    perimortem_fatal("Core Object clone received an invalid buffer contract");
  }

  const perimortem_count capacity = perimortem_core_object_capacity(payload);
  if (capacity == 0) {
    return 0;
  }

  const perimortem_count count = capacity / element_size;
  uint8_t* replacement =
      perimortem_core_object_allocate_buffer(descriptor, count, element_size);
  memcpy(replacement, payload, count * element_size);
  perimortem_core_object_release(payload);
  return replacement;
}

uint8_t* perimortem_core_object_reserve(
    uint8_t* payload,
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size,
    const uint8_t* default_value) {
  validate_buffer(descriptor, count, element_size, default_value);

  const perimortem_count current_capacity =
      perimortem_core_object_capacity(payload) / element_size;
  if (count <= current_capacity) {
    return payload;
  }

  uint8_t* replacement =
      perimortem_core_object_allocate_buffer(descriptor, count, element_size);
  const perimortem_count replacement_capacity =
      perimortem_core_object_capacity(replacement) / element_size;
  if (current_capacity != 0) {
    memcpy(replacement, payload, current_capacity * element_size);
  }

  for (perimortem_count index = current_capacity; index < replacement_capacity;
       ++index) {
    memcpy(replacement + index * element_size, default_value, element_size);
  }

  perimortem_core_object_release(payload);
  return replacement;
}
