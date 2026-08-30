// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stddef.h>
#include <stdint.h>

struct runtime_access_u64 {
  uint64_t* data;
  uint64_t size;
};

struct runtime_view_u64 {
  const uint64_t* data;
  uint64_t size;
};

uint64_t dense_storage[] = {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
static const uint64_t view_storage[] = {UINT64_C(5), UINT64_C(6)};
uint64_t printed_value = UINT64_C(0);
size_t printed_count = 0;

void ttx_system_print(uint64_t value) {
  printed_value = value;
  printed_count++;
}

struct runtime_access_u64 llvm_dense_access(void) {
  return (struct runtime_access_u64){
    .data = dense_storage,
    .size = sizeof(dense_storage) / sizeof(dense_storage[0]),
  };
}

struct runtime_view_u64 llvm_dense_view(void) {
  return (struct runtime_view_u64){
    .data = view_storage,
    .size = sizeof(view_storage) / sizeof(view_storage[0]),
  };
}

uint64_t llvm_object_identity(void* value) {
  return (uint64_t)(uintptr_t)value;
}
