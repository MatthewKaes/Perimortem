#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t library_native(void);

extern const uint64_t library_foreign_bias;
extern uint64_t library_foreign_state;
uint64_t library_foreign_add(uint64_t left, uint64_t right);

#ifdef __cplusplus
}
#endif
