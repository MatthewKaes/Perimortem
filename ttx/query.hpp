// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "ttx/abi.h"

namespace Ttx {

enum class Observation {
  Unknown,
  None,
  Resolved,
};

struct DomainObservation {
  Observation state;
  ttx_abstract domain;
  ttx_layout layout;
};

enum class CallableObservationState {
  Unknown,
  None,
  Resolved,
  SupportFailed,
};

struct CallableObservation {
  CallableObservationState state;
  ttx_callable callable;
  // A synthesized view may exist only during its result callback. The query
  // copies its two Layout snapshots there so the returned observation can be
  // used after that callback without retaining the producer's dispatch frame.
  std::shared_ptr<const ttx_callable_self> retained = {};
  ttx_pack_support_failure failure = TTX_PACK_SUPPORT_INVALID_LAYOUT;
};

struct RouteObservation {
  Observation state;
  ttx_route route;
  ttx_abstract candidate;
  ttx_borrowed_bytes bytes;
  std::shared_ptr<const ttx_route_self> retained = {};
};

struct ExtentObservation {
  Observation state;
  ttx_finite_extent extent;
  std::shared_ptr<const ttx_finite_extent_self> retained = {};
};

struct BytesObservation {
  Observation state;
  ttx_bytes bytes;
  std::shared_ptr<const ttx_bytes_self> retained = {};
};

enum class PackObservationState {
  Unknown,
  None,
  Packed,
  SupportFailed,
};

struct PackObservation {
  PackObservationState state;
  ttx_pack pack;
  ttx_pack_support_failure failure;
};

struct PackEntry {
  std::vector<uint8_t> path;
  ttx_abstract producer;
};

// These helpers collect one synchronous C ABI observation. Abstract answers
// remain borrowed graph identities. Typed support views may be temporary, so
// their exposed bytes, counts, or Layout snapshots are copied during the
// callback and retained by the returned observation. Requerying an owner
// creates another observation without rewriting the earlier projection.
auto resolve(ttx_abstract source) -> ttx_abstract;
auto resolve_concept(ttx_abstract source, ttx_borrowed_bytes route)
    -> ttx_abstract;
auto resolve_domain(ttx_abstract source) -> DomainObservation;
auto resolve_callable(ttx_abstract source) -> CallableObservation;
// Retain an identity free call shape while its provider callback is active.
auto retain_callable(ttx_callable source) -> CallableObservation;
auto resolve_route(ttx_abstract source) -> RouteObservation;
auto retain_route(ttx_route source) -> RouteObservation;
auto resolve_finite_extent(ttx_abstract source) -> ExtentObservation;
auto retain_finite_extent(ttx_finite_extent source) -> ExtentObservation;
auto resolve_bytes(ttx_abstract source) -> BytesObservation;
auto retain_bytes(ttx_bytes source) -> BytesObservation;
auto copy_bytes(ttx_abstract source) -> std::optional<std::vector<uint8_t>>;
auto relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation;
// Compares two Callable projections without assuming that either owner is a
// C++ Callable object. Parameters must admit flow in both directions because a
// caller and callee share that boundary, while candidate results flow into the
// required result shape. The resulting structural evidence still leaves each
// owner responsible for the behavior promised by its Callable contract.
auto compare_call_shape(ttx_abstract requirement, ttx_abstract candidate)
    -> ttx_interface_relation;
auto invoke(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context) -> PackObservation;
auto fit(ttx_layout receiving, ttx_pack source, ttx_context context)
    -> PackObservation;
auto pack(ttx_context context, ttx_layout produced_flow) -> PackObservation;
// Enumeration is support flow rather than another semantic query. This helper
// validates the complete synchronous protocol and returns the exact borrowed
// producers in structural order, allowing generic consumers to share one
// implementation without retaining the Layout or inventing a side record.
auto producers(ttx_pack pack) -> std::optional<std::vector<ttx_abstract>>;
auto pack_entries(ttx_pack pack) -> std::optional<std::vector<PackEntry>>;

}  // namespace Ttx
