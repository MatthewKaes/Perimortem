// Perimortem Engine
// Copyright © Matt Kaes

#include <stdint.h>

extern uint64_t ttx_package_native_default(void);
extern uint64_t ttx_package_native_supplied(void);
extern uint64_t ttx_package_native_static(void);

int main(void) {
  return ttx_package_native_default() == 12 &&
                 ttx_package_native_supplied() == 15 &&
                 ttx_package_native_static() == 40
             ? 0
             : 1;
}
