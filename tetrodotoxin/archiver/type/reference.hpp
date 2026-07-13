// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Archiver::Type {

// Compact type reference written into a package archive.
//
// Package buffers do not serialize pointers. Every type edge is tagged by the
// table it belongs to, then followed by the smallest index payload that can
// restore the real TTX address on read.
class Reference {
 public:
  enum class Kind : Bits_8 {
    None = 0,
    Builtin = 1,
    Local = 2,
    Package = 3,
  };

  Reference() = default;
  Reference(Count reference_id, Count type_id)
      : reference_id(reference_id), type_id(type_id) {}

  constexpr auto get_reference_id() const -> Count { return reference_id; }
  constexpr auto get_type_id() const -> Count { return type_id; }

 private:
  Count reference_id = Count(-1);
  Count type_id = Count(-1);
};

}  // namespace Tetrodotoxin::Archiver::Type
