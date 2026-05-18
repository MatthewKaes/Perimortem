// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

namespace Perimortem::Memory::Allocator {

// Used for only allocating memory which all shares the same lifetime enabling
// fast allocation / deallocation.
//
// Arenas avoid the book marking overhead of preface blocks so they are vastly
// more efficent if a large number of small objects need to be allocated rapidly
// which all share an assosiated lifetime (such as json deserialization).
//
// While constructors are called, deconstructors are never called for objects
// allocated out of an arena as it's assume they all get removed together when
// the arena's life time ends. Storing any Bibliotheca data in an arena will
// result in a leak until thread exit unless `Bibliotheca::remit` is explictly
// called on any rented data.
class Arena {
 public:
  // Attempt to request blocks in 32k pages including the preface and a previous
  // pointer.
  static constexpr Bits_64 page_size = (1 << 15);
  static constexpr Bits_64 arena_alignment = sizeof(Count);

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

    // Align the bump pointer to keep data produced aligned.
    Byte* root = rented_block + usage;
    usage = Core::Data::align<arena_alignment>(usage + bytes_requested);
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
