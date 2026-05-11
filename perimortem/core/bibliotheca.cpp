// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/bibliotheca.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;

// Gives the number of bits to shift to get the minimum containing size.
constexpr auto log2_pre_shift(Bits_64 value) -> Bits_64 {
  return 64 - __builtin_clzg(value - 1, Int(sizeof(Bits_64) * 8));
}

// If Count is 64 bits then limit us to some level below the 256 TB limits.
// 64 GB blocks is the current upper limit.
static constexpr Bits_8 max_radix = sizeof(Count) * 8 > 32 ? 36
                                                           : sizeof(Count) * 8;
static constexpr Bits_8 min_radix = log2_pre_shift(64);
static constexpr Count min_size = (1 << min_radix);
static constexpr Count radix_range = max_radix - min_radix;

struct alignas(64) Slab {
  static constexpr auto kilobytes_4 = 1 << 12;
  static constexpr auto megabytes_2 = kilobytes_4 << 9;
  static constexpr auto megabytes_2_mask = megabytes_2 - 1;

  // 32 MB chunks + 1 extra 2 MB chunk
  // The extra 2 MB chunk gives us wiggle room for all of the slab and preface
  // overhead which helps break up some particularly poor performing allocation
  // chunking based on Bibliotheca access patterns.
  static constexpr Count allocator_size = (megabytes_2 << 4) + megabytes_2;
  Count mapped_size;
  Count bump_ptr;
  Slab* ancestor;

  constexpr auto get_free_space() const -> Count {
    return mapped_size - bump_ptr;
  }

  // Allocates a chunk using a bump ptr. It's up to the Librarian to actually
  // dish out prefaced chunks and the Bibliotheca to manage chunk flows.
  //
  // We could do additional fancy alignment to detect TLB boundries, but this
  // is a micro optimization that slows down the common small chunk case.
  //
  // Note that all slabs from the Bibliotheca are `2 ^ N + 64` size where N is
  // at minimum 6 so allocs are always 64 byte aligned which is sufficient for
  // all resonable types so we never have to pay for caculating alignment.
  auto alloc(Count bytes) -> Byte* {
    auto location = reinterpret_cast<Byte*>(this) + bump_ptr;
    bump_ptr += bytes;
    return location;
  }
};

// Slab header should fill a single cache line.
static_assert(sizeof(Slab) == 64);

#ifdef PERI_LINUX
#include <sys/mman.h>

auto get_slab(Count block_size) -> Slab* {
  // Limit blocks to
  auto size = Slab::allocator_size;

  // If we need a specificly large slab then grab one but make sure it falls on
  // a 2MB boundary. Don't worry about fragmentation, as part of demotion the
  // left over bytes in the 2MB will be chunked eventually by smaller allocs.
  if (size < block_size) {
    const auto aligned = (~size + 1) & (Slab::megabytes_2_mask);
    size = block_size + aligned;
  }

  // Attempt to grab a block using huge TLB to speed up the slab allocator.
  auto constexpr page_access = PROT_READ | PROT_WRITE;
  auto constexpr page_flags = MAP_PRIVATE | MAP_ANONYMOUS;
  // Huge pages should be 2MB if available.
  // 1GB pages don't make sense for our usage.
  auto constexpr huge_page = MAP_HUGETLB | MAP_HUGE_2MB;
  Bool is_huge_page_optimized = true;
  auto ptr = mmap(nullptr, size, page_access, page_flags | huge_page, -1, 0);
  if (ptr == MAP_FAILED) {
    // Fall back to 4kb pages if the huge tlb failed.
    is_huge_page_optimized = false;
    ptr = mmap(nullptr, size, page_access, page_flags, -1, 0);

    // Check if a regular mmap also failed with regular 4kb pages.
    if (ptr == MAP_FAILED) {
      // TODO: Diagnostics
      return nullptr;
    }
  }

  Slab* slab = reinterpret_cast<Slab*>(ptr);
  slab->mapped_size = size;
  slab->ancestor = nullptr;
  slab->bump_ptr = sizeof(Slab);

  return slab;
}

auto release_slab(Slab* slab) -> Bool {
  auto success = munmap(slab, slab->allocator_size);
  if (success != 0) {
    // TODO: Diagnostics
    return false;
  }

  return true;
}
#endif

// Make sure Preface is always aligned so that the pointer returned is
// aligned.
class Preface final {
  friend Bibliotheca;

 public:
  auto get_usable_bytes() const -> Count { return block_size; }

 private:
  // The number of objects in this thread that have a reservation on
  // this block.
  Bits_64 reservations;
  // The size of usable bytes in the block.
  Bits_64 block_size;
  // Used for storing the next free block when stored in archives.
  Preface* next;
  // The archive index is an invariant of the block so store it in the header.
  Bits_64 archive_index;
#if PERI_DEBUG
  Bits_64 block_stamp;
  [[maybe_unused]] Bits_64 __reserved[1];
#elif
  [[maybe_unused]] Bits_64 __reserved[2];
#endif

  // Bibliotheca allocations reserve 16 bytes of "under_write" buffer.
  // This underwrite buffer is useful for optimizing certain system algorithms
  // that require a bit of underwriting (typically AVX).
  [[maybe_unused]] Bits_8
      __under_write_buffer[Bibliotheca::legal_underwrite_size];
};

// Blocks are always cached aligned by having the preface small allows us to be
static_assert(sizeof(Preface) == 64);

// Create a thread local static object that is fast to access.
// Use a POD type so we don't have to pay any of the initialization checks.
thread_local static struct {
  struct Collection {
    Preface* initial_entry;
    Bits_32 reserved_blocks;
    Bits_32 free_blocks;
  };

  Collection collections[radix_range];
  // The hirearchy of badness metrics from best to worst performance.
  Count check_out_requests;   // A checkout was requested
  Count allocation_requests;  // A new Preface was requested.
  Count demote_requests;
  Count slab_requests;
} secret_archive = {};

static_assert(sizeof(secret_archive) <= 512);

// Librarian's are responsible for managing the inventory of the thread.
// They are separate from the archive to keep them out of the loop for
// managing inflight blocks.
class Librarian {
 public:
  Slab* inventory = nullptr;

  // Avoids typical arena / slab allocation fragmentation by keeping slabs
  // arranged based on the amount of free memory allowing us to use left over
  // chunks greedily.
  //
  // The list is always sorted so a demotion is only O(n) time where N is the
  // approximate amount of memory used / ~32 MB, improving with larger
  // allocations.
  //
  // Still to maximize small chunk creation throughput (which is important
  // during thread start up when the Bibliotheca is empty), demote is only
  // called when we overrun the existing inventory page.
  void manage_inventory(Count bytes) {
    // Demote the active inventory to possibly inactive and see if we have
    // space in another block.
    Slab* current = inventory;
    if (current->ancestor) {
      secret_archive.demote_requests++;
      const auto free_space = current->get_free_space();

      // Bubble the page to it's new rank location to keep emptier pages at
      // the top of the inventory list.
      auto parent = &inventory;
      while (current->ancestor &&
             free_space < current->ancestor->get_free_space()) {
        auto next = current->ancestor;
        current->ancestor = next->ancestor;
        next->ancestor = current;

        *parent = next;
        parent = &(*parent)->ancestor;
      }
    }

    // If we now have room than we can avoid a slab allocation.
    if (bytes <= inventory->get_free_space()) [[likely]] {
      return;
    }

    // We were unable to find space so fetch a brand new slab and make it our
    // active inventory for allocations.
    secret_archive.slab_requests++;
    auto new_slab = get_slab(bytes);
    new_slab->ancestor = inventory;
    inventory = new_slab;
  }

  // Bytes requested is equal to the number of actual bytes + the size of the
  // preface header for the allocation.
  Preface* order_inventory(Count bytes) {
    secret_archive.allocation_requests++;

    // Check if our active inventory has space.
    if (bytes > inventory->get_free_space()) [[unlikely]] {
      manage_inventory(bytes);
    }

    Preface* entry = reinterpret_cast<Preface*>(inventory->alloc(bytes));
    return entry;
  }

  Librarian() {
    // Since we require at least some memory we can preallocate a block.
    // Having at least one block as an invariant speeds up the fast path.
    secret_archive.slab_requests++;
    inventory = get_slab(Slab::allocator_size);
  }

  // Forcefully reclaim all outstanding rentals since we are closing
  // out this Bibliotheca.
  ~Librarian() {
    while (inventory) {
      auto entry = inventory;
      inventory = inventory->ancestor;
      release_slab(entry);
    }
  }
};

constexpr auto caculate_archive_bucket(Count bytes) -> Bits_8 {
  return log2_pre_shift(bytes > min_size ? bytes : min_size);
}

constexpr auto archive_page_width(Bits_8 index) -> Count {
  return Count(1) << index;
}

// The preface is stored 16 bytes before the corpus block.
auto corpus_to_preface(Byte* entry) -> Preface* {
  return reinterpret_cast<Preface*>(entry) - 1;
}

// The preface is stored 16 bytes before the corpus block.
auto preface_to_corpus(Preface* entry) -> Byte* {
  return reinterpret_cast<Byte*>(entry + 1);
}

auto Bibliotheca::check_out(Count requested_bytes) -> Allocation {
  secret_archive.check_out_requests++;

  // Caculate the archive information for the request.
  const Count archive_bucket = caculate_archive_bucket(requested_bytes);
  const Count actual_bytes =
      archive_page_width(archive_bucket) + sizeof(Preface);
  const Count archive_index = archive_bucket - min_radix;

  // Slow path on a thread cache miss.
  if (secret_archive.collections[archive_index].initial_entry == nullptr) {
    // Since we are allocating memory we'll need to hire a librarian.
    thread_local static Librarian dave;

    // Order the requested block and log it's reservation in the archive for
    // accounting of usage.
    Preface* entry = dave.order_inventory(actual_bytes);
    secret_archive.collections[archive_index].reserved_blocks += 1;

#ifdef PERI_DEBUG
    if (entry == nullptr) [[unlikely]] {
      __builtin_debugtrap();
    }

    // Mark the block to detect bad remittances.
    entry->block_stamp = 'PERI';
#endif

    // Initialize the archive and reservation data.
    entry->archive_index = archive_index;
    entry->reservations = 1;
    entry->block_size = actual_bytes - sizeof(Preface);
    entry->next = nullptr;
    return Allocation{.ptr = preface_to_corpus(entry),
                      .capacity = entry->get_usable_bytes()};
  }

  Preface* entry = secret_archive.collections[archive_index].initial_entry;
  secret_archive.collections[archive_index].initial_entry = entry->next;
  secret_archive.collections[archive_index].free_blocks -= 1;

  // Rehydrate the reservation data.
  entry->reservations = 1;
  entry->next = nullptr;
  return Allocation{.ptr = preface_to_corpus(entry),
                    .capacity = entry->get_usable_bytes()};
}

auto Bibliotheca::reserve(Byte* data) -> Count {
  auto entry = corpus_to_preface(data);
  return entry->reservations++;
}

auto Bibliotheca::remit(Byte* data) -> Count {
  auto entry = corpus_to_preface(data);
  entry->reservations--;

#ifdef PERI_DEBUG
  // Detect if we underflowed on the reservation.
  if (entry->reservations > 1u << 30) [[unlikely]] {
    __builtin_debugtrap();
  }

  // Verify the entry signiture
  if (entry->block_stamp != 'PERI') [[unlikely]] {
    __builtin_debugtrap();
  }
#endif

  // If there are no reservations then return to the appropriate archive.
  if (entry->reservations == 0) {
    Count archive_index = entry->archive_index;

    entry->next = secret_archive.collections[archive_index].initial_entry;
    secret_archive.collections[archive_index].initial_entry = entry;
    secret_archive.collections[archive_index].free_blocks += 1;
    return 0;
  }

  return entry->reservations;
}

auto Bibliotheca::reserved_memory() -> Count {
  Count total_size = 0;
  for (int i = 0; i < radix_range; i++) {
    auto archive = secret_archive.collections[i];
    total_size += (Count(1) << (min_radix + i)) * archive.reserved_blocks;
  }

  return total_size;
}

auto Bibliotheca::free_memory() -> Count {
  Count total_size = 0;
  for (int i = 0; i < radix_range; i++) {
    auto archive = secret_archive.collections[i];
    total_size += (Count(1) << (min_radix + i)) * archive.free_blocks;
  }

  return total_size;
}

auto Bibliotheca::allocated_memory() -> Count {
  auto reserved = reserved_memory();
  auto free = free_memory();

  return reserved - free;
}

auto Bibliotheca::check_out_requests() -> Count {
  return secret_archive.check_out_requests;
}

auto Bibliotheca::allocation_requests() -> Count {
  return secret_archive.allocation_requests;
}

auto Bibliotheca::slab_requests() -> Count {
  return secret_archive.slab_requests;
}