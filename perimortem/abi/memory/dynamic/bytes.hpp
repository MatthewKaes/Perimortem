// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Abi::Memory::Dynamic {

// Bytes is the physical carrier used by generated code at a native boundary.
// The authored TTX Type owns behavior through Object[Unsigned_8], while this
// read only shape prevents ABI code from bypassing its mutation policy.
class Bytes {
 public:
  static constexpr auto create(const Unsigned_8* data, Count size) -> Bytes {
    Bytes result = {};
    result.data = data;
    result.size = size;
    return result;
  }

  constexpr auto get_data() const -> const Unsigned_8* { return data; }
  constexpr auto get_size() const -> Count { return size; }

 private:
  const Unsigned_8* data;
  Count size;
};

static_assert(sizeof(Bytes) == sizeof(Unsigned_8*) + sizeof(Count));
static_assert(alignof(Bytes) == alignof(Count));
static_assert(__is_trivial(Bytes));
static_assert(__is_standard_layout(Bytes));

}  // namespace Perimortem::Abi::Memory::Dynamic
