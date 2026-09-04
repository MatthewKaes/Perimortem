// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/c/value.h"

ttx_abstract TTX_CALL ttx_test_c_integer_create(
    ttx_abstract value,
    ttx_abstract view_bytes,
    ttx_abstract to_string) {
  static const uint8_t name[] = "C Integer(4)";
  static const uint8_t text[] = "4";
  return ttx_test_c_value_create(
      (ttx_borrowed_bytes){name, sizeof(name) - 1},
      (ttx_borrowed_bytes){text, sizeof(text) - 1}, value, view_bytes,
      to_string);
}
