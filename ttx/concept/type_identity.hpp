// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Ttx::Concept {

// Native inheritance checks compare a token for the actual C++ class. The
// linker unifies this local static across translation units in one linked
// image, but independently loaded modules need not share its address.
//
// Public binding uses the interface's declared UUID instead. Keeping the
// native proof separate prevents a foreign interface match from authorizing
// a cast to a C++ base subobject that the provider may not contain.
template <typename Target>
inline auto get_type_identity() -> ::U64 {
  static_assert(
      sizeof(__UINTPTR_TYPE__) <= sizeof(::U64),
      "A TTX type identity must retain every native pointer value.");

  static const U8 identity = 0;
  return static_cast<::U64>(reinterpret_cast<__UINTPTR_TYPE__>(&identity));
}

}  // namespace Ttx::Concept
