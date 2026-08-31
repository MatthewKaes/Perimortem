// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/aligned_buffer.h"

perimortem_bool perimortem_aligned_buffer_create(
    perimortem_count required_bytes,
    perimortem_count alignment,
    struct perimortem_aligned_buffer* result) {
  if (result == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0 ||
      alignment > PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT) {
    return PERIMORTEM_FALSE;
  }

  result->data = 0;
  result->capacity = 0;
  if (required_bytes == 0) {
    return PERIMORTEM_TRUE;
  }

  const struct perimortem_bibliotheca_allocation allocation =
      perimortem_bibliotheca_check_out(required_bytes);
  result->data = allocation.ptr;
  result->capacity = allocation.capacity;
  return PERIMORTEM_TRUE;
}

void perimortem_aligned_buffer_release(
    struct perimortem_aligned_buffer* buffer) {
  if (buffer == 0) {
    return;
  }

  perimortem_bibliotheca_remit(buffer->data);
  buffer->data = 0;
  buffer->capacity = 0;
}
