// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/bibliotheca.hpp"

namespace Perimortem::Memory {

// Used for only allocating memory which all shares the same lifetime enabling
// fast allocation / deallocation.
//
// While constructors are called deconstructors are never called for objects
// allocated out of a arena as it's assume they all get wiped together.
// go.
class Arena {
 public:
  // Attempt to request blocks in 16k pages including the preface.
  static constexpr uint64_t page_size =
      (1 << 14) - sizeof(Bibliotheca::Preface);
  static constexpr uint64_t alignment_filter = alignof(max_align_t) - 1;

  Arena();
  ~Arena();

  inline auto allocate(uint16_t bytes_requested) -> uint8_t* {
#ifdef PERI_DEBUG
    // You can't request more data than the size of a page.
    if (bytes_requested > page_size) {
      __builtin_debugtrap();
    }
#endif

    // Fetch a new page if we are full,
    if (rented_block->usage + bytes_requested > rented_block->capacity) {
      auto rent = Bibliotheca::check_out(page_size);

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

  auto reset() -> void;

  template <typename T>
  inline auto construct(uint32_t count = 1) -> T* {
    T* root = reinterpret_cast<T*>(allocate(sizeof(T) * count));
    for (int i = 0; i < count; i++) {
      new (root + i) T();
    }

    return root;
  }

 private:
  Bibliotheca::Preface* rented_block;
};

}  // namespace Perimortem::Memory
