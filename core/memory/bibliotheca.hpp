// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <stddef.h>
#include <array>

#include "core/memory/standard_types.hpp"

namespace Perimortem::Memory {

class Bibliotheca {
 public:
  Bibliotheca() = delete;

  using size_type = Bits_64;

  // Make sure Preface is always aligned so that the pointer returned is
  // aligned.
  struct alignas(size_in_bits<Bits_32>()) Preface {
    Preface* previous;
    size_type capacity;
    size_type usage;
    size_type reservations;
  };

  struct Archive {
    Preface* initial_entry = nullptr;
    size_type reserved_blocks = 0;
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

  static inline auto corpus_to_preface(Bits_8* entry) -> Preface* {
    return reinterpret_cast<Preface*>(entry - sizeof(Preface));
  }

  static auto check_out(size_type requested_bytes) -> Preface*;
  static auto reserve(Preface* entry) -> void;
  static auto remit(Preface* entry) -> size_type;
  static auto exchange(Preface* returning, Preface* reserving) -> void;

  static auto archive_sizes() -> std::array<uint64_t, radix_range>;
  static auto reserved_size() -> uint64_t;

 private:
  static std::array<Archive, radix_range> faceted_archives;
  static Bits_64 allocated_bytes;
};

}  // namespace Perimortem::Memory
