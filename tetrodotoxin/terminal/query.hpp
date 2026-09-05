// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vector>

#include "tetrodotoxin/terminal/provider.h"

namespace Tetrodotoxin::Terminal {

enum class BeginState { Requested, Failed, Invalid };

struct BeginObservation {
  BeginState state;
  tetrodotoxin_product_request request;
  ttx_abstract error;
};

enum class ProductState { Unknown, None, Failed, Produced, Invalid };

struct ProductObservation {
  ProductState state;
  ttx_pack products;
  tetrodotoxin_production_closure closure;
  ttx_abstract error;
};

enum class CancelState { Cancelled, AlreadySettled, Failed, Invalid };

struct CancelObservation {
  CancelState state;
  ttx_abstract error;
};

enum class ReobservePrepareState { Unavailable, Prepared, Failed, Invalid };

struct ReobservePrepareObservation {
  ReobservePrepareState state;
  tetrodotoxin_reobserve reobserve;
  ttx_abstract error;
};

enum class ReobserveCommitState { Changed, Armed, Failed, Invalid };

struct ReobserveCommitObservation {
  ReobserveCommitState state;
  ttx_abstract error;
};

enum class ReobserveCloseState { Closed, Failed, Invalid };

struct ReobserveCloseObservation {
  ReobserveCloseState state;
  ttx_abstract error;
};

struct AuthorityRevisionObservation {
  tetrodotoxin_authority_revision authority;
  uint64_t revision;
  bool current;
};

struct ClosureObservation {
  bool valid;
  std::vector<ttx_abstract> constants;
  std::vector<AuthorityRevisionObservation> authorities;
};

// These helpers own no semantic state. They only enforce the synchronous C
// result protocols and return the branch selected during this call, leaving
// the retained provider and request lifetimes with their actual owners.
auto begin(
    tetrodotoxin_terminal_provider provider,
    ttx_borrowed_bytes output_route,
    ttx_abstract product,
    tetrodotoxin_workspace_view workspace,
    ttx_abstract environment,
    ttx_abstract invocation) -> BeginObservation;
auto observe(tetrodotoxin_product_request request) -> ProductObservation;
auto cancel(tetrodotoxin_product_request request) -> CancelObservation;
auto prepare_reobserve(
    tetrodotoxin_product_request request,
    tetrodotoxin_reobserve_callback callback) -> ReobservePrepareObservation;
auto commit_reobserve(tetrodotoxin_reobserve reobserve)
    -> ReobserveCommitObservation;
auto close_reobserve(tetrodotoxin_reobserve reobserve)
    -> ReobserveCloseObservation;
auto observe_closure(tetrodotoxin_production_closure closure)
    -> ClosureObservation;

}  // namespace Tetrodotoxin::Terminal
