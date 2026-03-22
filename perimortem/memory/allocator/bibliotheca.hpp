// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <stddef.h>
#include <array>

#include "perimortem/memory/const/standard_types.hpp"

namespace Perimortem::Memory::Allocator {

// Thread local allocator for isolating and caching thread allocations.
//
// On x86_64 malloc will spit out pointers at 16 byte alignment, where due to
// the required Preface for the wide pointers objects from the Bibliotheca will
// be 8 byte aligned.
//
// At minimum a Bibliotheca object will take up 16 bytes with larger objects
// rounded up to the next power of 2.
//
// To fill a block use (2 ^ n - 8) for the allocation size. Allocating exact
// powers of 2 will actually waste ~50% of space due to the header bumping it to
// the next size.
//
// TODO: Slab allocate smaller partitions.
class Bibliotheca {
 public:
  Bibliotheca();
  ~Bibliotheca();

  using size_type = Bits_64;
  static constexpr Bits_8 required_alignment = 8;

  // Make sure Preface is always aligned so that the pointer returned is
  // aligned.
  class alignas(required_alignment) Preface {
    friend Bibliotheca;

   public:
    auto get_reservations() const -> Bits_32 { return reservations; }
    auto get_archive() const -> Bits_8 { return archive_index; }
    auto usable_bytes() const -> Bits_64 {
      return (static_cast<size_type>(1) << (min_radix + archive_index)) -
             sizeof(Preface);
    }

   private:
    union {
      // Used for chaining blocks when stored in archives.
      struct {
        Preface* next;
      };
      // Status of living blocks.
      struct {
        Bits_32 reservations;
        Bits_8 archive_index;
      };
    };
  };

  static constexpr auto size = sizeof(Preface);

  static_assert(sizeof(Preface) == required_alignment,
                "The size of Preface isn't equal to it's alignment");

  struct Archive {
    Preface* initial_entry = nullptr;
    size_type reserved_blocks = 0;
    size_type free_blocks = 0;
  };

  // Min radix is +1 power of two from just the header size. This means an
  // alocation can't be smaller than 64 bytes.
  static constexpr Bits_8 min_radix = std::bit_width(sizeof(Preface));
  static constexpr Bits_8 max_radix = sizeof(size_type) * 8 - 1;
  static constexpr size_type radix_range = max_radix - min_radix;
  static constexpr size_type min_size = static_cast<size_type>(1) << min_radix;
  static constexpr size_type max_size = static_cast<size_type>(1) << max_radix;

  static inline auto preface_to_corpus(Preface* entry) -> Bits_8* {
    return reinterpret_cast<Bits_8*>(entry) + sizeof(Preface);
  }

  static auto check_out(size_type requested_bytes) -> Preface*;
  static auto reserve(Preface* entry) -> void;
  static auto remit(Preface* entry) -> size_type;
  static auto exchange(Preface* returning, Preface* reserving) -> void;

  static auto reserved_memory() -> std::array<size_type, radix_range>;
  static auto free_memory() -> std::array<size_type, radix_range>;

 private:
  std::array<Archive, radix_range> faceted_archives;
};

}  // namespace Perimortem::Memory::Allocator
