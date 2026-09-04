// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

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

struct CallableObservation {
  Observation state;
  ttx_callable callable;
};

struct RouteObservation {
  Observation state;
  ttx_abstract candidate;
  ttx_borrowed_bytes bytes;
};

struct ExtentObservation {
  Observation state;
  ttx_finite_extent extent;
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

// These helpers complete mandatory synchronous sink calls without introducing
// another graph object or retaining an answer beyond the current observation.
// They are the C++ spelling of the C ABI protocol rather than another semantic
// resolution path.
auto resolve(ttx_abstract source) -> ttx_abstract;
auto resolve_concept(ttx_abstract source, ttx_borrowed_bytes route)
    -> ttx_abstract;
auto resolve_domain(ttx_abstract source) -> DomainObservation;
auto resolve_callable(ttx_abstract source) -> CallableObservation;
auto resolve_route(ttx_abstract source) -> RouteObservation;
auto resolve_finite_extent(ttx_abstract source) -> ExtentObservation;
auto relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation;
auto fit(ttx_layout receiving, ttx_pack source, ttx_context context)
    -> PackObservation;
auto pack(ttx_context context, ttx_layout produced_flow) -> PackObservation;

}  // namespace Ttx
