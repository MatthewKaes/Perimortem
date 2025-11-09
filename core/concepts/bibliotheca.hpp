// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <array>
#include <cstdint>
#include <stddef.h>

namespace Perimortem::Concepts {

class Bibliotheca {
 public:
  Bibliotheca() = delete;

  using size_type = uint32_t;
  static constexpr uint8_t min_radix = 6;
  static constexpr uint8_t max_radix = 24;
  static constexpr size_type radix_range = max_radix - min_radix;
  static constexpr size_type min_size = 1 << min_radix;
  static constexpr size_type max_size = 1 << max_radix;

#if PERI_DEBUG
  static constexpr uint64_t biblio_marker = 0x6269626c696f6d65;
#endif

  // Make sure Preface is always aligned so that the pointer returned is
  // aligned.
  struct alignas(alignof(max_align_t)) Preface {
#if PERI_DEBUG
    uint64_t marker;
#endif
    Preface* previous;
    size_type capacity;
    size_type usage;
    size_type reservations;
  };

  struct Archive {
    Preface* initial_entry = nullptr;
    uint32_t reserved_blocks = 0;
  };

  static inline auto preface_to_corpus(Preface* entry) -> uint8_t* {
    return reinterpret_cast<uint8_t*>(entry) + entry->usage;
  }

  static auto check_out(size_type requested_bytes) -> Preface*;
  static auto reserve(Preface* entry) -> void;
  static auto remit(Preface* entry) -> size_type;

  static auto archive_sizes() -> std::array<uint64_t, radix_range>;
  static auto reserved_size() -> uint64_t;

 private:
  static std::array<Archive, radix_range> faceted_archives;
  static uint64_t allocated_bytes;
};

}  // namespace Perimortem::Concepts
