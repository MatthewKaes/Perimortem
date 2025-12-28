// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/bibliotheca.hpp"
#include "memory/byte_view.hpp"

namespace Perimortem::Storage::Base64 {

class Decoded {
 public:
  Decoded(const Memory::ByteView& source);
  Decoded(const Decoded& rhs);
  ~Decoded();

  inline auto get_view() const -> Memory::ByteView {
    return Memory::ByteView(text, size);
  }

 private:
  char* text;
  uint64_t size;
  Memory::Bibliotheca::Preface* rented_block;
};

}  // namespace Perimortem::Storage::Base64
