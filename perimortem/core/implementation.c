// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/implementation.h"

static void clear(struct perimortem_implementation* implementation) {
  implementation->object = 0;
  implementation->projection = 0;
}

perimortem_bool perimortem_core_implementation_retain(
    uint8_t* object,
    const struct perimortem_projection* projection,
    struct perimortem_implementation* result) {
  if (result == 0 || object == 0 || projection == 0) {
    return PERIMORTEM_FALSE;
  }

  perimortem_core_object_retain(object);
  result->object = object;
  result->projection = projection;
  return PERIMORTEM_TRUE;
}

void perimortem_core_implementation_adopt(
    uint8_t* object,
    const struct perimortem_projection* projection,
    struct perimortem_implementation* result) {
  if (result == 0) {
    return;
  }

  result->object = object;
  result->projection = projection;
}

void perimortem_core_implementation_copy(
    const struct perimortem_implementation* source,
    struct perimortem_implementation* result) {
  if (result == 0) {
    return;
  }

  if (source == 0) {
    clear(result);
    return;
  }

  perimortem_core_object_retain(source->object);
  *result = *source;
}

void perimortem_core_implementation_assign(
    struct perimortem_implementation* target,
    const struct perimortem_implementation* source) {
  if (target == 0 || target == source) {
    return;
  }

  struct perimortem_implementation replacement = {0};
  perimortem_core_implementation_copy(source, &replacement);
  perimortem_core_implementation_release(target);
  *target = replacement;
}

void perimortem_core_implementation_move(
    struct perimortem_implementation* target,
    struct perimortem_implementation* source) {
  if (target == 0 || target == source) {
    return;
  }

  perimortem_core_implementation_release(target);
  if (source == 0) {
    return;
  }

  *target = *source;
  clear(source);
}

void perimortem_core_implementation_release(
    struct perimortem_implementation* implementation) {
  if (implementation == 0) {
    return;
  }

  perimortem_core_object_release(implementation->object);
  clear(implementation);
}

perimortem_bool perimortem_core_implementation_is_empty(
    const struct perimortem_implementation* implementation) {
  return implementation == 0 || implementation->object == 0;
}

perimortem_bool perimortem_core_implementation_is_valid(
    const struct perimortem_implementation* implementation) {
  return implementation != 0 && implementation->object != 0 &&
         implementation->projection != 0;
}
