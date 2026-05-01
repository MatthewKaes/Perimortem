// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"

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
  static constexpr Bits_64 page_size =
      (1 << 15) - (sizeof(Bibliotheca::Preface) * 2);
  static constexpr Bits_64 alignment_filter = sizeof(Bibliotheca::Preface) - 1;

  Arena();
  Arena(Arena&& arena);
  ~Arena();

  inline auto allocate(Bits_64 bytes_requested) -> Byte* {
    // Fetch a new page if we are full due to either running out of our current
    // page, or needing to allocate an object larger than our page size.
    if (usage + bytes_requested > page_size) {
      auto rent = Bibliotheca::check_out(page_size);

      // Store the previous pointer in the arena itself.
      // [Preface] [Preface*] [Data ... ]
      Bibliotheca::Preface** previous =
          Core::Data::cast<Bibliotheca::Preface*>(
              Bibliotheca::preface_to_corpus(rent));

      // Store and swap the blocks.
      *previous = rented_block;
      rented_block = rent;

      // Bump the usage so we don't overwrite the old block.
      usage = sizeof(Bibliotheca::Preface*);
    }

    Byte* root = Bibliotheca::preface_to_corpus(rented_block) + usage;
    usage += bytes_requested;

    // Align the pointer to keep it aligned.
    auto required_alignment = (~usage + 1) & (alignment_filter);
    usage += required_alignment;

    return root;
  }

  // Creates a basic value type object but does not construct it.
  template <typename T>
  inline auto allocate() -> T& {
    return *Core::Data::cast<T>(allocate(sizeof(T)));
  }

  auto reset() -> void;

 private:
  Bibliotheca::Preface* rented_block;
  Count usage;
};

}  // namespace Perimortem::Memory::Allocator
