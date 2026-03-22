// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/view/bytes.hpp"

namespace Perimortem::Storage::Base64 {

class Decoded {
 public:
  Decoded(Memory::Allocator::Arena& arena, const Memory::View::Bytes source);
  Decoded(const Decoded& rhs);

  inline auto get_view() const -> const Memory::View::Bytes { return result; }

 private:
  Memory::View::Bytes result;
};

}  // namespace Perimortem::Storage::Base64
