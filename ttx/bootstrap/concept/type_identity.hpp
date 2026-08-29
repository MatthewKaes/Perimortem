// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Ttx::Concept {

// Returns one opaque identity for a C++ type in the current program. The local
// static is shared by every translation unit that instantiates the same type,
// while distinct specializations own distinct live objects. Encoding its
// address as an integer keeps callers from treating the carrier as an object.
// This identity ends with the process and must never enter a durable format.
template <typename Target>
inline auto get_type_identity() -> ::U64 {
  static_assert(
      sizeof(__UINTPTR_TYPE__) <= sizeof(::U64),
      "A TTX type identity must retain every native pointer value.");

  static const U8 identity = 0;
  return static_cast<::U64>(reinterpret_cast<__UINTPTR_TYPE__>(&identity));
}

}  // namespace Ttx::Concept
