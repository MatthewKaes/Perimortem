// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/static/vector.hpp"

namespace Perimortem::Memory::Allocator {

// Thread local allocator for isolating and caching thread allocations.
//
// Any memory fetched from the Bibliotheca is guaranteed to be cleaned
// up on thread exit. To ensure correct cleanup ordering any thread_local
// objects should be created with `Singleton`
// (perimortem/memory/managed/singleton.hpp)
//
class Bibliotheca final {
 private:
  static constexpr Bits_8 required_alignment = 16;
  // If Count is 64 bits then limit us to some level below the 256 TB limits.
  // 64 GB blocks is the current upper limit.
  static constexpr Bits_8 max_radix = sizeof(Count) * 8 > 32
                                          ? 36
                                          : sizeof(Count) * 8;
  static constexpr Bits_8 min_radix = __builtin_ctzl(required_alignment << 1);
  static constexpr Count min_size = 1 << min_radix;
  static constexpr Count max_size = static_cast<Count>(1) << max_radix;

 public:
  // The number of powers of two the allocator spans.
  static constexpr Count radix_range = max_radix - min_radix;

  // Make sure Preface is always aligned so that the pointer returned is
  // aligned.
  class alignas(required_alignment) Preface final {
    friend Bibliotheca;

   public:
    auto get_reservations() const -> Bits_32 { return rented.reservations; }
    auto get_archive() const -> Bits_8 { return rented.archive_index; }
    auto get_usable_bytes() const -> Count {
      return (static_cast<Count>(1) << (rented.archive_index + min_radix)) -
             sizeof(Preface);
    }

   private:
    // The preface can be in two states, rented or stored.
    // To keep the size to 8 bytes use a union that is only changed when
    // switching states.
    union {
      Preface* ancestor;
      // Status of archived blocks.
      struct {
        // Used for storing the next free block when stored in archives.
        Preface* next;
      } stored;
      // Status of living blocks.
      struct {
        // The number of objects in this thread that have a reservation on
        // this block.
        Bits_32 reservations;
        // The archive index is technically an invariant of the block when
        // created but blocks forget their size when they are stored.
        // The archive they are stored in carries that information which
        // requires the value to be rehydrated whenever a block is rented.
        Bits_8 archive_index;
      } rented;
    };
  };

  static_assert(sizeof(Preface) == required_alignment,
                "The size of Preface isn't equal to it's alignment");

  static inline auto preface_to_corpus(Preface* entry) -> Bits_8* {
    return reinterpret_cast<Bits_8*>(entry) + sizeof(Preface);
  }

  static inline auto corpus_to_preface(Bits_8* entry) -> Preface* {
    return reinterpret_cast<Preface*>(entry - sizeof(Preface));
  }

  static inline auto get_memory_consumption(Preface* entry) -> Count {
    if (!entry) {
      return 0;
    }

    return (static_cast<Count>(1) << (entry->rented.archive_index + min_radix));
  }

  // Creates a free entry which can be used.
  static auto check_out(Count requested_bytes) -> Preface*;

  // Adds a reservation to the block.
  static auto reserve(Preface* entry) -> void;

  // Removes a reservation from the block.
  // If the number of reservations is zero then the block is checked in to the
  // Bibliotheca for future use.
  static auto remit(Preface* entry) -> Count;

  // Remits a block while reserving another block in a single transaction.
  // If the returning block and reserving block are the same then exchanged is
  // guaranteed to not load the actual block.
  static auto exchange(Preface* returning, Preface* reserving) -> void;

  static auto reserved_memory() -> Core::Static::Vector<Count, radix_range>;
  static auto free_memory() -> Core::Static::Vector<Count, radix_range>;
  static auto allocated_memory() -> Count;
  static auto check_out_requests() -> Count;
  static auto allocation_requests() -> Count;
};

}  // namespace Perimortem::Memory::Allocator
