// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_BIBLIOTHECA_H
#define PERIMORTEM_CORE_BIBLIOTHECA_H

#include "perimortem/core/object.h"
#include "perimortem/core/perimortem.h"

enum {
  PERIMORTEM_BIBLIOTHECA_LEGAL_UNDERWRITE_SIZE = 16,
  PERIMORTEM_BIBLIOTHECA_ALLOCATION_ALIGNMENT = 64,
};

#define PERIMORTEM_BIBLIOTHECA_MAXIMUM_CAPACITY (UINT64_C(1) << 35)

// Bibliotheca is the thread memory cache used by the TTX graph and the
// Tetrodotoxin runtime. Like the thread caches in jemalloc it keeps ordinary
// allocation free from synchronization. Unlike a general purpose allocator it
// retains every free block and mapped slab until the owning thread closes.
// Stable compiler and frame workloads can reuse those size classes cheaply,
// while a service with irregular peaks may retain far more memory than its
// current working set.
//
// Bibliotheca does not compact live storage or purge unused pages during a
// thread lifetime. Doing either would add metadata, synchronization, or
// relocation policy to a runtime whose graph and frame identities are expected
// to remain stable. Workloads that need bounded retention should use a general
// allocator instead of treating this cache as one.
//
// Requests round up to power of two classes from 64 bytes through 32 GiB. Each
// block also carries one 64 byte preface, so tiny and uneven allocations trade
// substantial space for alignment and predictable reuse. Mimalloc and jemalloc
// use denser size classes and return unused pages to the operating system.
// Bibliotheca deliberately favors stable addresses and repeatable frame costs
// instead.
//
// A block belongs to the thread that checked it out. Retain and remit use that
// same thread because another cache cannot safely retain a pointer into the
// original thread's slabs. Read only views may cross a worker call while their
// owner remains alive, but ownership never crosses with them.
struct perimortem_bibliotheca_allocation {
  uint8_t* ptr;
  perimortem_count capacity;
};

// Sixteen bytes immediately before the allocation remain available to
// Perimortem algorithms that knowingly use underwriting. This is useful for
// vectorized decoders, but it is not compatible with an ordinary malloc
// pointer and makes Bibliotheca unsuitable as a transparent system allocator.
PERIMORTEM_EXTERN_C struct perimortem_bibliotheca_allocation
    perimortem_bibliotheca_check_out(perimortem_count requested_bytes);

PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_reserve(uint8_t* entry);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_reservation_count(uint8_t* entry);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_capacity(uint8_t* entry);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_remit(uint8_t* entry);

// Object descriptors share the allocator preface so the one word Object
// carrier does not need another allocation header. The typed relationship is
// intentional runtime policy rather than generic allocator metadata.
PERIMORTEM_EXTERN_C void perimortem_bibliotheca_bind_object(
    uint8_t* entry,
    const struct perimortem_object_descriptor* descriptor);
PERIMORTEM_EXTERN_C const struct perimortem_object_descriptor*
    perimortem_bibliotheca_get_object(uint8_t* entry);

// These values describe cached corpus capacity. They exclude prefaces, unused
// slab tails, and operating system page state, so they are useful for comparing
// TTX allocation behavior rather than reporting process resident memory.
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_reserved_memory(void);
PERIMORTEM_EXTERN_C perimortem_count perimortem_bibliotheca_free_memory(void);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_allocated_memory(void);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_check_out_requests(void);
PERIMORTEM_EXTERN_C perimortem_count
    perimortem_bibliotheca_allocation_requests(void);
PERIMORTEM_EXTERN_C perimortem_count perimortem_bibliotheca_slab_requests(void);

// Generated global destruction and slab reclamation share one thread owner.
// Cleanup runs in reverse registration order before any slab is unmapped. This
// explicit order replaces reliance on unrelated C++ thread local destructors.
PERIMORTEM_EXTERN_C void perimortem_core_cleanup_register(
    void (*destructor)(void));
PERIMORTEM_EXTERN_C void perimortem_bibliotheca_close_thread(void);

#endif
