// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

namespace Perimortem::Memory::Allocator {

// Used for only allocating memory which all shares the same lifetime enabling
// fast allocation / deallocation.
//
// While constructors are called deconstructors are never called for objects
// allocated out of a arena as it's assume they all get wiped together.
// go.
class Arena {
 public:
  // Attempt to request blocks in 32k pages including the preface and a previous
  // pointer.
  static constexpr Bits_64 page_size = (1 << 15);
  static constexpr Bits_64 alignment_filter = sizeof(Count) - 1;

  Arena();
  ~Arena();
  Arena(Arena& arena) = delete;
  Arena(Arena&& arena) = delete;

  inline auto allocate(Count bytes_requested) -> Byte* {
    // Fetch a new page if we are full due to either running out of our current
    // page, or needing to allocate an object larger than our page size.
    //
    // Arena's are meant to be quick and scrapy do they don't do any of the page
    // demotion that the Bibliotheca performs. Long lived Arena's most likely
    // will cause fragmentation issues so use them only for short lifetimes.
    if (usage + bytes_requested > page_size) {
      fetch_page(bytes_requested);
    }

    Byte* root = rented_block + usage;
    usage += bytes_requested;

    // Align the pointer to keep it aligned.
    auto required_alignment = (~usage + 1) & (alignment_filter);
    usage += required_alignment;

    return root;
  }

  // Creates a basic value type object but does not construct it.
  template <typename T>
  auto allocate() -> T& {
    return *Core::Data::cast<T>(allocate(sizeof(T)));
  }

  auto reset() -> void;

 private:
  auto fetch_page(Count bytes_requested) -> void;

  Byte* rented_block;
  Count usage;
};

}  // namespace Perimortem::Memory::Allocator
