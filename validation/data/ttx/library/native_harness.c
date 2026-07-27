#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

extern uint64_t library_native(void);

const uint64_t library_foreign_bias = UINT64_C(2);
uint64_t library_foreign_state = UINT64_C(0);

uint64_t library_foreign_add(uint64_t left, uint64_t right) {
  return left + right;
}

int main(void) {
  const uint64_t first = library_native();
  const uint64_t second = library_native();

  printf("%" PRIu64 " %" PRIu64 "\n", first, second);
  return first == 22 && second == 42 && library_foreign_state == UINT64_C(40)
             ? 0
             : 1;
}
