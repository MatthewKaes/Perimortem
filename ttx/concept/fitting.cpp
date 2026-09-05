// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/fitting.hpp"

#include "ttx/query.hpp"

using namespace Ttx;

auto Ttx::fit_leaf(ttx_abstract producer, ttx_abstract requirement) -> LeafFit {
  const bool requirement_unknown =
      ttx_abstract_same(requirement, ttx_unknown());
  const bool producer_unknown = ttx_abstract_same(producer, ttx_unknown());
  if (requirement_unknown && producer_unknown) {
    return LeafFit::Accepted;
  }
  if (requirement_unknown || producer_unknown ||
      ttx_abstract_same(requirement, ttx_none()) ||
      ttx_abstract_same(producer, ttx_none())) {
    return requirement_unknown || producer_unknown ? LeafFit::Unknown
                                                   : LeafFit::None;
  }

  const DomainObservation required_domain = Ttx::resolve_domain(requirement);
  const bool requires_domain =
      required_domain.state == Observation::Resolved &&
      ttx_abstract_same(required_domain.domain, requirement);
  const DomainObservation produced_domain = Ttx::resolve_domain(producer);
  if (requires_domain && produced_domain.state == Observation::Resolved &&
      ttx_abstract_same(produced_domain.domain, requirement)) {
    return LeafFit::Accepted;
  }

  const ttx_interface_relation proof = Ttx::relation(producer, requirement);
  if (proof == TTX_INTERFACE_SATISFIED || proof == TTX_INTERFACE_EQUIVALENT) {
    return LeafFit::Accepted;
  }
  if (proof == TTX_INTERFACE_UNKNOWN ||
      (requires_domain && produced_domain.state == Observation::Unknown)) {
    return LeafFit::Unknown;
  }
  return LeafFit::None;
}
