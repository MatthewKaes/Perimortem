// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <inttypes.h>
#include <stdint.h>

#include "Foreign/c_abi.h"

const uint64_t library_foreign_bias = UINT64_C(2);
uint64_t library_foreign_state = UINT64_C(0);

uint64_t library_foreign_add(uint64_t left, uint64_t right) {
  return left + right;
}

int run_foreign_integration(void) {
  const uint64_t first = TTX_FUNC_library_5fnative_static();
  const uint64_t second = TTX_FUNC_library_5fnative_static();
  return first == UINT64_C(22) && second == UINT64_C(42) &&
                 library_foreign_state == UINT64_C(40) &&
                 TTX_FUNC_cross_5fartifact_static() == UINT64_C(15)
             ? 0
             : 1;
}
