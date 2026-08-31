// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/object.h"

#include "perimortem/core/bibliotheca.h"
#include "perimortem/core/diagnostics/fatal.h"

static void validate_descriptor(
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size) {
  if (descriptor == 0 || descriptor->size == 0 || descriptor->alignment == 0 ||
      (descriptor->alignment & (descriptor->alignment - 1)) != 0 ||
      descriptor->alignment > PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT ||
      descriptor->finalize == 0 || count == 0 ||
      element_size != descriptor->size || count > UINT64_MAX / element_size) {
    perimortem_fatal("Core Object received an invalid runtime descriptor");
  }
}

static uint8_t* allocate_object(
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size) {
  validate_descriptor(descriptor, count, element_size);
  const struct perimortem_bibliotheca_allocation allocation =
      perimortem_bibliotheca_check_out(count * element_size);
  perimortem_bibliotheca_bind_object(allocation.ptr, descriptor);
  return allocation.ptr;
}

uint8_t* perimortem_core_object_allocate(
    const struct perimortem_object_descriptor* descriptor) {
  if (descriptor == 0) {
    perimortem_fatal("Core Object received an empty descriptor");
  }

  return allocate_object(descriptor, 1, descriptor->size);
}

uint8_t* perimortem_core_object_allocate_buffer(
    const struct perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size) {
  return allocate_object(descriptor, count, element_size);
}

void perimortem_core_object_retain(uint8_t* payload) {
  if (payload == 0) {
    return;
  }

  if (perimortem_bibliotheca_get_object(payload) == 0) {
    perimortem_fatal(
        "Core Object received a handle without its runtime descriptor");
  }

  perimortem_bibliotheca_reserve(payload);
}

void perimortem_core_object_release(uint8_t* payload) {
  if (payload == 0) {
    return;
  }

  const struct perimortem_object_descriptor* descriptor =
      perimortem_bibliotheca_get_object(payload);
  if (descriptor == 0) {
    perimortem_fatal(
        "Core Object received a handle without its runtime descriptor");
  }

  if (perimortem_bibliotheca_reservation_count(payload) == 1) {
    descriptor->finalize(payload);
  }

  perimortem_bibliotheca_remit(payload);
}

perimortem_count perimortem_core_object_capacity(uint8_t* payload) {
  return perimortem_bibliotheca_capacity(payload);
}

perimortem_count perimortem_core_object_reservations(uint8_t* payload) {
  return perimortem_bibliotheca_reservation_count(payload);
}

const struct perimortem_object_descriptor* perimortem_core_object_descriptor(
    uint8_t* payload) {
  return perimortem_bibliotheca_get_object(payload);
}

void perimortem_core_object_finalize_trivial(uint8_t* payload) {
  (void)payload;
}
