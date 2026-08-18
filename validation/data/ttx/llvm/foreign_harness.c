#include <inttypes.h>
#include <stdint.h>

#include "Foreign/c_abi.h"

const uint64_t library_foreign_bias = UINT64_C(2);
uint64_t library_foreign_state = UINT64_C(0);

uint64_t library_foreign_add(uint64_t left, uint64_t right) {
  return left + right;
}

int run_foreign_integration(void) {
  const uint64_t first = library_native();
  const uint64_t second = library_native();
  return first == UINT64_C(22) && second == UINT64_C(42) &&
                 library_foreign_state == UINT64_C(40) &&
                 library_cross_artifact() == UINT64_C(15)
             ? 0
             : 1;
}
