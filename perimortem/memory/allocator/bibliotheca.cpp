// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/allocator/bibliotheca.hpp"

#include "perimortem/core/math.hpp"

#include <bit>

using namespace Perimortem::Memory::Allocator;

thread_local static Bibliotheca isolated;

// If a short lived thread was created we'll need to release all blocks.
Bibliotheca::~Bibliotheca() {
  for (Count i = 0; i < faceted_archives.get_size(); i++) {
    while (faceted_archives[i].initial_entry) {
      auto entry = faceted_archives[i].initial_entry;
      faceted_archives[i].initial_entry = entry->next;
      free(entry);
    }
  }
}

auto Bibliotheca::check_out(size_type requested_bytes) -> Preface* {
  // Ensure a minimum page size and that we have room for the Preface reserved.
  // Round up to the nearest power of 2.
  const size_type actual_bytes = Core::Math::max(
      std::bit_ceil(requested_bytes + static_cast<size_type>(sizeof(Preface))),
      min_size);

  // Since we ensure that bytes is a power of two we can extract the index
  // by quickly taking the width.
  Bits_8 archive_index = std::bit_width(actual_bytes) - min_radix;

  // Slow path on a thread cache miss.
  if (isolated.faceted_archives[archive_index].initial_entry == nullptr) {
    // TODO: Keeping malloc is useful so we are some what portable with the
    // underlying memory system, but we should be slab allocating from malloc
    // since anything using the Bibliotheca is expected to be reusable.
    //
    // Then smaller (more common) archive chunks can just be passed out from
    // A set of stack allocator.
    Preface* entry = static_cast<Preface*>(malloc(actual_bytes));

    // Update accounting of usage.
    isolated.faceted_archives[archive_index].reserved_blocks += 1;

#if PERI_DEBUG
    if (entry == nullptr) [[unlikely]] {
      __builtin_debugtrap();
    }
#endif

    entry->archive_index = archive_index;
    entry->reservations = 1;
    return entry;
  }

  Preface* entry = isolated.faceted_archives[archive_index].initial_entry;
  isolated.faceted_archives[archive_index].initial_entry = entry->next;

  entry->archive_index = archive_index;
  entry->reservations = 1;
  return entry;
}

auto Bibliotheca::reserve(Preface* entry) -> void {
  entry->reservations++;
}

auto Bibliotheca::remit(Preface* entry) -> size_type {
  entry->reservations--;

#if PERI_DEBUG
  // Detect if we underflowed on the reservation.
  if (entry->reservations > 1u << 30) [[unlikely]] {
    __builtin_debugtrap();
  }
#endif

  // If there are no reservations then return to the appropriate archive.
  if (entry->reservations == 0) {
    Byte archive_index = entry->archive_index;

    entry->next = isolated.faceted_archives[archive_index].initial_entry;
    isolated.faceted_archives[archive_index].initial_entry = entry;
    isolated.faceted_archives[archive_index].free_blocks += 1;
  }

  return entry->reservations;
}

auto Bibliotheca::exchange(Preface* returning, Preface* reserving) -> void {
  // Avoid locking on cycling
  if (returning == reserving)
    return;

  // Exchange
  reserving->reservations++;
  returning->reservations--;

  // If there are no reservations then return to the appropriate archive.
  if (returning->reservations <= 0) {
#if PERI_DEBUG
    // We should never remit an checkout more than it's been reserved.
    if (returning->reservations < 0) [[unlikely]] {
      __builtin_debugtrap();
    }
#endif

    Byte archive_index = returning->archive_index;

    returning->next = isolated.faceted_archives[archive_index].initial_entry;
    isolated.faceted_archives[archive_index].initial_entry = returning;
    isolated.faceted_archives[archive_index].free_blocks += 1;
  }
}

auto Bibliotheca::reserved_memory() -> Static::Vector<size_type, radix_range> {
  Static::Vector<size_type, radix_range> sizes;
  for (int i = 0; i < isolated.faceted_archives.get_size(); i++) {
    auto archive = isolated.faceted_archives[i];
    if (archive.initial_entry == nullptr) {
      sizes[i] = 0;
      continue;
    }

    sizes[i] = static_cast<size_type>(1)
               << (min_radix + i) * archive.reserved_blocks;
  }

  return sizes;
}

auto Bibliotheca::free_memory() -> Static::Vector<size_type, radix_range> {
  Static::Vector<size_type, radix_range> sizes;
  for (int i = 0; i < isolated.faceted_archives.get_size(); i++) {
    auto archive = isolated.faceted_archives[i];
    if (archive.initial_entry == nullptr) {
      sizes[i] = 0;
      continue;
    }

    sizes[i] = static_cast<size_type>(1)
               << (min_radix + i) * archive.free_blocks;
  }

  return sizes;
}