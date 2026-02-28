// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"
#include "memory/view/bytes.hpp"

namespace Perimortem::Storage::Base64 {

class Decoded {
 public:
  Decoded(Memory::Arena& arena, const Memory::View::Bytes source);
  Decoded(const Decoded& rhs);

  inline auto get_view() const -> const Memory::View::Bytes {
    return result;
  }

 private:
  Memory::View::Bytes result;
};

}  // namespace Perimortem::Storage::Base64
