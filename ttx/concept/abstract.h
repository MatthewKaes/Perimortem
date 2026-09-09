// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ABSTRACT_H
#define TTX_CONCEPT_ABSTRACT_H

#include "perimortem/core/perimortem.h"
#include "perimortem/system/uuid.h"
#include "ttx/concept/binding.h"
#include "ttx/concept/documentation.h"

#define TTX_ABSTRACT_ID_HIGH 0x01a084b0c85e7b94ULL
#define TTX_ABSTRACT_ID_LOW 0xb1794f55bff0faa6ULL

#ifdef __cplusplus
extern "C" {
#endif

// A graph can contain providers that have no native C++ Abstract base. Keeping
// the receiver separate from its operations lets those providers answer from
// their own storage, without allocating an adapter object for every graph
// edge. Consumers traverse the supplied operations instead of depending on
// the language or class that implements the subject.
//
// The supplying boundary establishes the exact Abstract contract before the
// table is used, since an unknown table cannot safely describe how to call
// itself. That agreement includes all four operations and the documentation
// view they return. This representation uses the platform C ABI and has been
// exercised on Linux with 64 bit pointers.
//
// The pair borrows its provider so graph owners can manage many views under
// one lifetime. Source is nonnull and identifies the subject for that promised
// lifetime, but retaining the token alone keeps neither state nor code alive.
// The caller retains their lifetime owner, including the providers needed by
// returned handles.
typedef struct ttx_abstract {
  const void* source;
  const struct ttx_abstract_ops* operations;
} ttx_abstract;

typedef struct ttx_abstract_ops {
  // Separating selection from value production lets a consumer obtain an
  // interface without triggering its computation or data transfers. Bind
  // therefore selects an implementation without invoking its value operations.
  // Result points to writable caller storage and is written only for Satisfied.
  // An Abstract request returns this same pair because the supplying boundary
  // has already established that contract.
  ttx_binding_status (*bind)(const void* source, perimortem_uuid requested,
                             ttx_binding* result);

  // Borrowed names and documentation let tools inspect the subject without
  // building a separate metadata object. The name remains readable for this
  // Abstract's promised lifetime, and documentation carries its own line view.
  perimortem_view_bytes (*get_name)(const void* source);
  ttx_documentation (*get_documentation)(const void* source);

  // The selected subject brings its own operation table, allowing resolution
  // to cross between different provider representations. It satisfies this
  // same Abstract contract, so the consumer can continue without recovering
  // a native class or reinterpreting the original receiver's storage.
  ttx_abstract (*resolve)(const void* source);
} ttx_abstract_ops;

#ifdef __cplusplus
}
#endif

#endif
