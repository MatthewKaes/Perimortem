// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/bibliotheca.hpp"

#include <memory>

namespace Perimortem::Memory {

// Used for only allocating memory which all shares the same lifetime enabling
// fast allocation / deallocation.
//
// While constructors are called deconstructors are never called for objects
// allocated out of a arena as it's assume they all get wiped together.
// go.
class Arena {
 public:
  // Attempt to request blocks in 32k pages including the preface.
  static constexpr Bits_64 page_size =
      (1 << 15) - sizeof(Bibliotheca::Preface);
  static constexpr Bits_64 alignment_filter = alignof(max_align_t) - 1;

  Arena();
  ~Arena();

  inline auto allocate(Bits_64 bytes_requested) -> Byte* {
    // Fetch a new page if we are full,
    if (rented_block->usage + bytes_requested > rented_block->capacity) {
      auto rent = Bibliotheca::check_out(std::max(page_size, bytes_requested));

      // Add the block to our our stack and make it top most block.
      rent->previous = rented_block;
      rented_block = rent;
    }

    uint8_t* root = Bibliotheca::preface_to_corpus(rented_block);
    rented_block->usage += bytes_requested;

    // Align the page.
    auto required_alignment = (~rented_block->usage + 1) & (alignment_filter);
    rented_block->usage += required_alignment;

    return root;
  }

  // Creates a basic value type object.
  template <typename T>
  inline auto allocate() -> T& {
    return *reinterpret_cast<T*>(allocate(sizeof(T)));
  }

  auto reset() -> void;

 private:
  Bibliotheca::Preface* rented_block;
};

}  // namespace Perimortem::Memory
