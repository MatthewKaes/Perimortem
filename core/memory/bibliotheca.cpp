// Perimortem Engine
// Copyright © Matt Kaes

#include "core/memory/bibliotheca.hpp"

#include <stdlib.h>
#include <atomic>

// All Archive opperations are supposed to be short lived so we can use a lock
// free atomic spin.
class ArchiveLock {
 public:
  ArchiveLock() {
    while (lock.test_and_set(std::memory_order_acquire)) {
    }
  }

  ~ArchiveLock() {
    lock.clear();
  }

 private:
  inline static std::atomic_flag lock = ATOMIC_FLAG_INIT;
};

using namespace Perimortem::Memory;

std::array<Bibliotheca::Archive, Bibliotheca::radix_range>
    Bibliotheca::faceted_archives;
uint64_t Bibliotheca::allocated_bytes;

auto Bibliotheca::check_out(size_type requested_bytes) -> Preface* {
  const ArchiveLock lock;

  // Ensure a minimum page size and that we have room for the Preface reserved.
  // Round up to the nearest power of 2.
  const uint32_t actual_bytes = std::max(
      std::bit_ceil(requested_bytes + static_cast<size_type>(sizeof(Preface))),
      min_size);

  // Since we ensure that bytes is a power of two we can extract the index
  // by quickly taking the width.
  uint8_t archive_index = std::bit_width(actual_bytes) - min_radix;

  if (actual_bytes >= max_size ||
      faceted_archives[archive_index].initial_entry == nullptr) {
    Preface* entry = static_cast<Preface*>(malloc(actual_bytes));

#if PERI_DEBUG
    if (entry == nullptr) [[unlikely]] {
      __builtin_debugtrap();
    }

    entry->marker = biblio_marker;
#endif
    entry->previous = nullptr;
    entry->usage = sizeof(Preface);
    entry->capacity = actual_bytes;
    entry->reservations = 1;

    allocated_bytes += actual_bytes;

    return entry;
  }

  Preface* entry = faceted_archives[archive_index].initial_entry;
  faceted_archives[archive_index].initial_entry = entry->previous;

  entry->previous = nullptr;
  entry->usage = sizeof(Preface);
  entry->reservations = 1;
  return entry;
}

auto Bibliotheca::reserve(Preface* entry) -> void {
  const ArchiveLock lock;
  entry->reservations++;
}

auto Bibliotheca::remit(Preface* entry) -> size_type {
  const ArchiveLock lock;
  entry->reservations--;

  // If there are no reservations then return to the appropriate archive.
  if (entry->reservations == 0) {
    uint8_t archive_index = std::bit_width(entry->capacity) - min_radix;
    entry->previous = faceted_archives[archive_index].initial_entry;
    faceted_archives[archive_index].initial_entry = entry;
  }

  return entry->reservations;
}

auto Bibliotheca::archive_sizes() -> std::array<uint64_t, radix_range> {
  const ArchiveLock lock;
  std::array<uint64_t, radix_range> sizes;
  for (int i = 0; i < faceted_archives.size(); i++) {
    auto archive = faceted_archives[i];
    if (archive.initial_entry == nullptr) {
      sizes[i] = 0;
      continue;
    }

    sizes[i] = archive.initial_entry->capacity * archive.reserved_blocks;
  }

  return sizes;
}

auto Bibliotheca::reserved_size() -> uint64_t {
  return allocated_bytes;
}
