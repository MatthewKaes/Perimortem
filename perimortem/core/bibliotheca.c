// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/bibliotheca.h"

#include "perimortem/core/diagnostics/fatal.h"

#if !defined(PERI_LINUX)
#error Bibliotheca needs a platform mapping and thread teardown implementation
#endif

#include <linux/mman.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

enum {
  minimum_radix = 6,
  maximum_radix = 35,
  radix_count = maximum_radix - minimum_radix + 1,
};

static const perimortem_count slab_alignment = UINT64_C(1) << 21;
static const perimortem_count ordinary_slab_size =
    (UINT64_C(32) << 20) + (UINT64_C(2) << 20);

struct perimortem_bibliotheca_thread;

// A slab uses one cache line before its corpus. Mappings already satisfy the
// required alignment, while the explicit padding keeps the C representation
// independent from a compiler alignment extension.
struct perimortem_bibliotheca_slab {
  perimortem_count mapped_size;
  perimortem_count bump;
  struct perimortem_bibliotheca_slab* ancestor;
  uint8_t reserved[40];
};

// Every allocation starts after one cache line preface. The final sixteen
// bytes are the underwriting promised by the public contract. Object metadata
// occupies its own typed field so algorithms retain the complete underwriting
// region.
struct perimortem_bibliotheca_preface {
  perimortem_count reservations;
  perimortem_count block_size;
  struct perimortem_bibliotheca_preface* next;
  perimortem_count archive_index;
  const struct perimortem_object_descriptor* object_descriptor;
  struct perimortem_bibliotheca_thread* owner;
  uint8_t underwrite[PERIMORTEM_BIBLIOTHECA_LEGAL_UNDERWRITE_SIZE];
};

struct perimortem_bibliotheca_collection {
  struct perimortem_bibliotheca_preface* initial_entry;
  uint32_t reserved_blocks;
  uint32_t free_blocks;
};

// The archive remains exactly eight cache lines. Its direct thread storage
// avoids initialization guards on the allocation path and keeps the hottest
// size class heads close together.
struct perimortem_bibliotheca_archive {
  struct perimortem_bibliotheca_collection collections[radix_count];
  perimortem_count check_out_requests;
  perimortem_count allocation_requests;
  perimortem_count demote_requests;
  perimortem_count slab_requests;
};

struct perimortem_cleanup_entry {
  void (*destructor)(void);
  struct perimortem_cleanup_entry* previous;
};

// Cleanup and slab inventory share one thread state because their destruction
// order is semantic. Separate POSIX keys would run in an unspecified order and
// could unmap Object storage before generated destructors used it.
struct perimortem_bibliotheca_thread {
  struct perimortem_bibliotheca_archive archive;
  struct perimortem_bibliotheca_slab* inventory;
  struct perimortem_cleanup_entry* cleanup;
  perimortem_bool registered;
};

_Static_assert(
    sizeof(struct perimortem_bibliotheca_slab) ==
        PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT,
    "Bibliotheca slab header must fill one cache line");
_Static_assert(
    sizeof(struct perimortem_bibliotheca_preface) ==
        PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT,
    "Bibliotheca preface must fill one cache line");
_Static_assert(
    sizeof(struct perimortem_bibliotheca_archive) == 512,
    "Bibliotheca archive should remain eight cache lines");

static _Thread_local struct perimortem_bibliotheca_thread thread_memory;
static pthread_key_t thread_memory_key;
static pthread_once_t thread_memory_once = PTHREAD_ONCE_INIT;

static perimortem_count align_up(
    perimortem_count value,
    perimortem_count alignment) {
  const perimortem_count remainder = value & (alignment - 1);
  return remainder == 0 ? value : value + alignment - remainder;
}

static uint8_t containing_radix(perimortem_count value) {
  return (uint8_t)(64 - __builtin_clzll((unsigned long long)(value - 1)));
}

static perimortem_count slab_free_space(
    const struct perimortem_bibliotheca_slab* slab) {
  return slab->mapped_size - slab->bump;
}

static struct perimortem_bibliotheca_slab* create_slab(
    perimortem_count block_size) {
  perimortem_count size = ordinary_slab_size;
  if (size < block_size) {
    if (block_size > UINT64_MAX - slab_alignment) {
      perimortem_fatal("Bibliotheca slab size overflowed");
    }

    size = align_up(block_size, slab_alignment);
  }

  // Huge pages can reduce translation pressure for stable compiler and frame
  // workloads. They also consume scarce physical memory in large increments,
  // so failure falls back to ordinary demand paged mappings rather than making
  // huge page availability part of the runtime contract.
  const int access = PROT_READ | PROT_WRITE;
  const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  const int huge_pages = MAP_HUGETLB | MAP_HUGE_2MB;
  void* mapping = mmap(0, size, access, flags | huge_pages, -1, 0);
  if (mapping == MAP_FAILED) {
    mapping = mmap(0, size, access, flags, -1, 0);
  }

  if (mapping == MAP_FAILED) {
    perimortem_fatal("Bibliotheca was unable to allocate memory from the OS");
  }

  struct perimortem_bibliotheca_slab* slab = mapping;
  slab->mapped_size = size;
  slab->bump = sizeof(*slab);
  slab->ancestor = 0;
  return slab;
}

static void release_slab(struct perimortem_bibliotheca_slab* slab) {
  if (munmap(slab, slab->mapped_size) != 0) {
    perimortem_fatal("Bibliotheca was unable to release memory to the OS");
  }
}

static uint8_t* allocate_from_slab(
    struct perimortem_bibliotheca_slab* slab,
    perimortem_count bytes) {
  uint8_t* location = (uint8_t*)slab + slab->bump;
  slab->bump += bytes;
  return location;
}

static struct perimortem_bibliotheca_preface* corpus_to_preface(
    uint8_t* entry) {
  return ((struct perimortem_bibliotheca_preface*)entry) - 1;
}

static uint8_t* preface_to_corpus(
    struct perimortem_bibliotheca_preface* entry) {
  return (uint8_t*)(entry + 1);
}

static void validate_owner(
    const struct perimortem_bibliotheca_thread* state,
    const struct perimortem_bibliotheca_preface* entry) {
  if (entry->owner != state) {
    perimortem_fatal("Bibliotheca received an allocation from another thread");
  }

  if (entry->archive_index >= radix_count) {
    perimortem_fatal("Bibliotheca allocation metadata was corrupted");
  }
}

// POSIX fixes thread storage destruction to an erased pointer callback. The
// boundary converts that pointer immediately to Bibliotheca state and never
// exposes erased context through a Perimortem operation table or Callable.
static void close_thread_memory(void* selected);

static void close_main_memory(void) {
  close_thread_memory(&thread_memory);
}

static void initialize_thread_memory_key(void) {
  if (pthread_key_create(&thread_memory_key, close_thread_memory) != 0) {
    perimortem_fatal("Bibliotheca could not create thread storage");
  }

  if (atexit(close_main_memory) != 0) {
    perimortem_fatal("Bibliotheca could not register main thread cleanup");
  }
}

static struct perimortem_bibliotheca_thread* get_thread_memory(void) {
  if (pthread_once(&thread_memory_once, initialize_thread_memory_key) != 0) {
    perimortem_fatal("Bibliotheca could not initialize thread storage");
  }

  if (!thread_memory.registered) {
    if (pthread_setspecific(thread_memory_key, &thread_memory) != 0) {
      perimortem_fatal("Bibliotheca could not retain thread storage");
    }

    thread_memory.registered = PERIMORTEM_TRUE;
  }

  return &thread_memory;
}

// Slabs are ordered by remaining tail space. This does not compact a slab or
// recover holes because individual free blocks already live in their size
// class lists. The ordering only lets new prefaces consume useful slab tails
// before another mapping is requested.
static void manage_inventory(
    struct perimortem_bibliotheca_thread* state,
    perimortem_count bytes) {
  struct perimortem_bibliotheca_slab* current = state->inventory;
  if (current->ancestor != 0) {
    ++state->archive.demote_requests;
    const perimortem_count free_space = slab_free_space(current);
    struct perimortem_bibliotheca_slab** parent = &state->inventory;

    while (current->ancestor != 0 &&
           free_space < slab_free_space(current->ancestor)) {
      struct perimortem_bibliotheca_slab* next = current->ancestor;
      current->ancestor = next->ancestor;
      next->ancestor = current;
      *parent = next;
      parent = &next->ancestor;
    }
  }

  if (bytes <= slab_free_space(state->inventory)) {
    return;
  }

  ++state->archive.slab_requests;
  struct perimortem_bibliotheca_slab* slab = create_slab(bytes);
  slab->ancestor = state->inventory;
  state->inventory = slab;
}

static struct perimortem_bibliotheca_preface* order_inventory(
    struct perimortem_bibliotheca_thread* state,
    perimortem_count bytes) {
  ++state->archive.allocation_requests;
  if (bytes > slab_free_space(state->inventory)) {
    manage_inventory(state, bytes);
  }

  return (struct perimortem_bibliotheca_preface*)allocate_from_slab(
      state->inventory, bytes);
}

static void initialize_inventory(struct perimortem_bibliotheca_thread* state) {
  if (state->inventory != 0) {
    return;
  }

  ++state->archive.slab_requests;
  state->inventory = create_slab(ordinary_slab_size);
}

static perimortem_count remit_from_thread(
    struct perimortem_bibliotheca_thread* state,
    uint8_t* data) {
  if (data == 0) {
    return 0;
  }

  struct perimortem_bibliotheca_preface* entry = corpus_to_preface(data);
  validate_owner(state, entry);
  if (entry->reservations == 0) {
    perimortem_fatal("Bibliotheca allocation was remitted more than retained");
  }

  --entry->reservations;
  if (entry->reservations != 0) {
    return entry->reservations;
  }

  struct perimortem_bibliotheca_collection* collection =
      &state->archive.collections[entry->archive_index];
  entry->next = collection->initial_entry;
  collection->initial_entry = entry;
  ++collection->free_blocks;
  return 0;
}

static void close_thread_memory(void* selected) {
  struct perimortem_bibliotheca_thread* state = selected;
  if (state == 0 || !state->registered) {
    return;
  }

  // Generated destructors run while every Object payload is still mapped.
  // Registering another cleanup from a destructor remains visible to this loop
  // and preserves reverse registration order.
  while (state->cleanup != 0) {
    struct perimortem_cleanup_entry* entry = state->cleanup;
    state->cleanup = entry->previous;
    entry->destructor();
    remit_from_thread(state, (uint8_t*)entry);
  }

  while (state->inventory != 0) {
    struct perimortem_bibliotheca_slab* slab = state->inventory;
    state->inventory = slab->ancestor;
    release_slab(slab);
  }

  pthread_setspecific(thread_memory_key, 0);
  memset(state, 0, sizeof(*state));
}

struct perimortem_bibliotheca_allocation perimortem_bibliotheca_check_out(
    perimortem_count requested_bytes) {
  if (requested_bytes > PERIMORTEM_BIBLIOTHECA_MAXIMUM_CAPACITY) {
    perimortem_fatal("Bibliotheca request exceeded the 32 GiB size classes");
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  initialize_inventory(state);
  ++state->archive.check_out_requests;

  perimortem_count requested = requested_bytes;
  if (requested < PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT) {
    requested = PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT;
  }

  const uint8_t archive_bucket = containing_radix(requested);
  const perimortem_count archive_index = archive_bucket - minimum_radix;
  const perimortem_count capacity = UINT64_C(1) << archive_bucket;
  const perimortem_count actual_bytes =
      capacity + sizeof(struct perimortem_bibliotheca_preface);
  struct perimortem_bibliotheca_collection* collection =
      &state->archive.collections[archive_index];

  struct perimortem_bibliotheca_preface* entry = collection->initial_entry;
  if (entry != 0) {
    collection->initial_entry = entry->next;
    --collection->free_blocks;
  } else {
    entry = order_inventory(state, actual_bytes);
    if (collection->reserved_blocks == UINT32_MAX) {
      perimortem_fatal("Bibliotheca size class block count overflowed");
    }

    ++collection->reserved_blocks;
    entry->archive_index = archive_index;
    entry->block_size = capacity;
    entry->owner = state;
  }

  entry->reservations = 1;
  entry->object_descriptor = 0;
  entry->next = 0;

  const struct perimortem_bibliotheca_allocation allocation = {
    .ptr = preface_to_corpus(entry),
    .capacity = entry->block_size,
  };

  return allocation;
}

perimortem_count perimortem_bibliotheca_reserve(uint8_t* data) {
  if (data == 0) {
    perimortem_fatal("Bibliotheca cannot retain an empty allocation");
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  struct perimortem_bibliotheca_preface* entry = corpus_to_preface(data);
  validate_owner(state, entry);
  if (entry->reservations == UINT64_MAX) {
    perimortem_fatal("Bibliotheca allocation reservation count overflowed");
  }

  return ++entry->reservations;
}

perimortem_count perimortem_bibliotheca_reservation_count(uint8_t* data) {
  if (data == 0) {
    return 0;
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  struct perimortem_bibliotheca_preface* entry = corpus_to_preface(data);
  validate_owner(state, entry);
  return entry->reservations;
}

perimortem_count perimortem_bibliotheca_capacity(uint8_t* data) {
  if (data == 0) {
    return 0;
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  struct perimortem_bibliotheca_preface* entry = corpus_to_preface(data);
  validate_owner(state, entry);
  return entry->block_size;
}

void perimortem_bibliotheca_bind_object(
    uint8_t* data,
    const struct perimortem_object_descriptor* descriptor) {
  if (data == 0 || descriptor == 0) {
    perimortem_fatal(
        "Bibliotheca received an invalid Object descriptor binding");
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  struct perimortem_bibliotheca_preface* entry = corpus_to_preface(data);
  validate_owner(state, entry);
  if (entry->object_descriptor != 0) {
    perimortem_fatal(
        "Bibliotheca received a duplicate Object descriptor binding");
  }

  entry->object_descriptor = descriptor;
}

const struct perimortem_object_descriptor* perimortem_bibliotheca_get_object(
    uint8_t* data) {
  if (data == 0) {
    return 0;
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  struct perimortem_bibliotheca_preface* entry = corpus_to_preface(data);
  validate_owner(state, entry);
  return entry->object_descriptor;
}

perimortem_count perimortem_bibliotheca_remit(uint8_t* data) {
  return remit_from_thread(get_thread_memory(), data);
}

perimortem_count perimortem_bibliotheca_reserved_memory(void) {
  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  perimortem_count total = 0;
  for (uint8_t index = 0; index < radix_count; ++index) {
    total += (UINT64_C(1) << (minimum_radix + index)) *
             state->archive.collections[index].reserved_blocks;
  }

  return total;
}

perimortem_count perimortem_bibliotheca_free_memory(void) {
  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  perimortem_count total = 0;
  for (uint8_t index = 0; index < radix_count; ++index) {
    total += (UINT64_C(1) << (minimum_radix + index)) *
             state->archive.collections[index].free_blocks;
  }

  return total;
}

perimortem_count perimortem_bibliotheca_allocated_memory(void) {
  return perimortem_bibliotheca_reserved_memory() -
         perimortem_bibliotheca_free_memory();
}

perimortem_count perimortem_bibliotheca_check_out_requests(void) {
  return get_thread_memory()->archive.check_out_requests;
}

perimortem_count perimortem_bibliotheca_allocation_requests(void) {
  return get_thread_memory()->archive.allocation_requests;
}

perimortem_count perimortem_bibliotheca_slab_requests(void) {
  return get_thread_memory()->archive.slab_requests;
}

void perimortem_core_cleanup_register(void (*destructor)(void)) {
  if (destructor == 0) {
    perimortem_fatal("Bibliotheca cannot register an empty destructor");
  }

  struct perimortem_bibliotheca_thread* state = get_thread_memory();
  const struct perimortem_bibliotheca_allocation allocation =
      perimortem_bibliotheca_check_out(sizeof(struct perimortem_cleanup_entry));
  struct perimortem_cleanup_entry* entry =
      (struct perimortem_cleanup_entry*)allocation.ptr;
  entry->destructor = destructor;
  entry->previous = state->cleanup;
  state->cleanup = entry;
}

void perimortem_bibliotheca_close_thread(void) {
  if (thread_memory.registered) {
    close_thread_memory(&thread_memory);
  }
}
