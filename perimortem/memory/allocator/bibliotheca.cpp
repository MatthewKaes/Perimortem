// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/allocator/bibliotheca.hpp"

#include "perimortem/core/data_model.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem;
using namespace Perimortem::Memory::Allocator;

// Create a thread local static object that is fast to access.
// Use a POD type so we don't have to pay any of the initialization checks.
thread_local static struct {
  struct Collection {
    Bibliotheca::Preface* initial_entry = nullptr;
    Bits_32 reserved_blocks = 0;
    Bits_32 free_blocks = 0;
  };

  Collection collections[Bibliotheca::radix_range] = {};
} secret_archive;

static_assert(sizeof(secret_archive) == 496);

constexpr auto caculate_archive_bucket(Count bytes) -> Bits_8 {
  return Core::size_in_bits<Count>() -
         __builtin_clzl(bytes + sizeof(Bibliotheca::Preface) - 1u);
}

constexpr auto archive_page_width(Bits_8 index) -> Count {
  return Count(1) << index;
}

auto Bibliotheca::check_out(Count requested_bytes) -> Preface* {
  // Librarian's are responsible for managing the inventory of the thread.
  // They are separate from the archive to keep them out of the loop for
  // managing inflight blocks.
  class Librarian {
   public:
    Preface* inventory = nullptr;

    Preface* order_inventory(Count bytes) {
      Preface* entry = static_cast<Preface*>(malloc(bytes));
      entry->ancestor = inventory;
      inventory = entry;

      return entry;
    }

    // Forcefully reclaim all outstanding rentals since we are closing
    // out this Bibliotheca.
    ~Librarian() {
      while (inventory) {
        auto entry = inventory;
        inventory = inventory->ancestor;
        free(entry);
      }
    }
  };

  // Caculate the archive information for the request.
  const Bits_8 archive_bucket = caculate_archive_bucket(requested_bytes);
  const Count actual_bytes = archive_page_width(archive_bucket);
  const Bits_8 archive_index = archive_bucket - min_radix;

  // Slow path on a thread cache miss.
  if (secret_archive.collections[archive_index].initial_entry == nullptr) {
    // Since we are allocating memory we'll need to hire a librarian.
    thread_local static Librarian dave;

    // Order the requested block and log it's reservation in the archive for
    // accounting of usage.
    Preface* entry = dave.order_inventory(actual_bytes);
    secret_archive.collections[archive_index].reserved_blocks += 1;

#if PERI_DEBUG
    if (entry == nullptr) [[unlikely]] {
      __builtin_debugtrap();
    }
#endif

    // Initialize the archive and reservation data.
    entry->rented.archive_index = archive_index;
    entry->rented.reservations = 1;
    return entry;
  }

  Preface* entry = secret_archive.collections[archive_index].initial_entry;
  secret_archive.collections[archive_index].initial_entry = entry->stored.next;

  // Rehydrate the archive and reservation data.
  entry->rented.archive_index = archive_index;
  entry->rented.reservations = 1;
  return entry;
}

auto Bibliotheca::reserve(Preface* entry) -> void {
  entry->rented.reservations++;
}

auto Bibliotheca::remit(Preface* entry) -> Count {
  entry->rented.reservations--;

#if PERI_DEBUG
  // Detect if we underflowed on the reservation.
  if (entry->rented.reservations > 1u << 30) [[unlikely]] {
    __builtin_debugtrap();
  }
#endif

  // If there are no reservations then return to the appropriate archive.
  if (entry->rented.reservations == 0) {
    Byte archive_index = entry->rented.archive_index;

    entry->stored.next = secret_archive.collections[archive_index].initial_entry;
    secret_archive.collections[archive_index].initial_entry = entry;
    secret_archive.collections[archive_index].free_blocks += 1;
  }

  return entry->rented.reservations;
}

auto Bibliotheca::exchange(Preface* returning, Preface* reserving) -> void {
  // Avoid locking on cycling
  if (returning == reserving)
    return;

  // Exchange
  reserving->rented.reservations++;
  returning->rented.reservations--;

  // If there are no reservations then return to the appropriate archive.
  if (returning->rented.reservations <= 0) {
#if PERI_DEBUG
    // We should never remit an checkout more than it's been reserved.
    if (returning->rented.reservations < 0) [[unlikely]] {
      __builtin_debugtrap();
    }
#endif

    Byte archive_index = returning->rented.archive_index;

    returning->stored.next = secret_archive.collections[archive_index].initial_entry;
    secret_archive.collections[archive_index].initial_entry = returning;
    secret_archive.collections[archive_index].free_blocks += 1;
  }
}

auto Bibliotheca::reserved_memory() -> Static::Vector<Count, radix_range> {
  Static::Vector<Count, radix_range> sizes;
  for (int i = 0; i < radix_range; i++) {
    auto archive = secret_archive.collections[i];
    if (archive.initial_entry == nullptr) {
      sizes[i] = 0;
      continue;
    }

    sizes[i] = static_cast<Count>(1)
               << (min_radix + i) * archive.reserved_blocks;
  }

  return sizes;
}

auto Bibliotheca::free_memory() -> Static::Vector<Count, radix_range> {
  Static::Vector<Count, radix_range> sizes;
  for (int i = 0; i < radix_range; i++) {
    auto archive = secret_archive.collections[i];
    if (archive.initial_entry == nullptr) {
      sizes[i] = 0;
      continue;
    }

    sizes[i] = static_cast<Count>(1) << (min_radix + i) * archive.free_blocks;
  }

  return sizes;
}