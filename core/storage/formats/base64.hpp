// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/bibliotheca.hpp"
#include "memory/managed_string.hpp"

namespace Perimortem::Storage::Base64 {

class Decoded {
 public:
  Decoded(const Memory::ManagedString& source, bool disable_vectorization = false);
  Decoded(const Decoded& rhs);
  ~Decoded();

  inline auto get_view() const -> Memory::ManagedString {
    return Memory::ManagedString(text, size);
  }

 private:
  char* text;
  uint64_t size;
  Memory::Bibliotheca::Preface* rented_block;
};

}  // namespace Perimortem::Storage::Base64
