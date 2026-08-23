// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stddef.h>

#include "SystemAbi/c_abi.h"

_Static_assert(
    sizeof(ttx_systemabi_Bytes) == 16,
    "Dynamic::Bytes carrier changed");
_Static_assert(
    offsetof(ttx_systemabi_Bytes, data) == 0,
    "Bytes data offset changed");
_Static_assert(
    offsetof(ttx_systemabi_Bytes, size) == 8,
    "Bytes size offset changed");
_Static_assert(
    sizeof(ttx_systemabi_Option_5bBytes_5d) == 24,
    "Option[Dynamic::Bytes] carrier changed");
_Static_assert(
    offsetof(ttx_systemabi_Option_5bBytes_5d, set) == 16,
    "Option[Dynamic::Bytes] state offset changed");

int main(void) {
  return llvm_system_roundtrip() ? 0 : 1;
}
