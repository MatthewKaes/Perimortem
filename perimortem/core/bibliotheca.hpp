// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core {

// The primary memory manager used by all systems in the Perimortem engine.
//
// Provides Thread local allocator for isolating and caching thread allocations.
// Can be mixed with the standard library but using STL objects with the
// Bibliotheca is ill-advised.
//
// Any memory fetched from the Bibliotheca is guaranteed to be cleaned up on
// thread exit. Until thread exit memory is perserved and is allocated into
// power of 2 chunks.
//
// For ideal performance threads should have stable allocation and deallocation
// patterns but this isn't a hard requirement.
class Bibliotheca final {
 public:
  // The legal amount that algorithms are able to underwrite the allocated
  // buffer.
  //
  // For x86_64 the underwrite buffer is guaranteed to be on a separate cache
  // line from the data pointer address making it can be useful for storing
  // infrequently used data related to the allocation or for algorithms that
  // avoid a copy by using a bit of the underwrite buffer.
  //
  // Algorithms and types that use the underwrite buffer are incompatable with
  // the C++ standard library so it should be used sparingly.
  static constexpr auto legal_underwrite_size = 16;

  struct Allocation {
    Bits_8* ptr;
    Count capacity;
  };

  // Creates a free entry which can be used.
  static auto check_out(Count requested_bytes) -> Allocation;

  // Adds a reservation to the block.
  static auto reserve(Bits_8* entry) -> Count;

  // Removes a reservation from the block.
  // If the number of reservations is zero then the block is checked in to the
  // Bibliotheca for future use.
  static auto remit(Bits_8* entry) -> Count;

  // Methods for analyzing the state of the Bibliotheca.
  static auto reserved_memory() -> Count;
  static auto free_memory() -> Count;
  static auto allocated_memory() -> Count;
  static auto check_out_requests() -> Count;
  static auto allocation_requests() -> Count;
  static auto slab_requests() -> Count;
};

}  // namespace Perimortem::Core
