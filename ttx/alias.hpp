// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abstract.hpp"

namespace Ttx {

// Alias is transparent indirection to one required referent. Unknown records
// that the referent has not settled yet, while an exact target commits this
// identity permanently. None is rejected because absence cannot satisfy the
// promise that an Alias has a referent.
//
// Alias forwards every observation and offers no contract of its own. A caller
// therefore cannot inspect or negotiate the indirection, and an implementation
// cannot hide policy on the forwarding boundary.
class Alias final : public Abstract {
 public:
  Alias();
  explicit Alias(ttx_abstract target);

  auto bind(ttx_abstract target) -> bool;

  auto name() const -> ttx_borrowed_bytes override;
  auto documentation(ttx_abstract self) const -> ttx_documentation override;
  auto resolve(ttx_abstract self) const -> ttx_abstract override;
  auto resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract override;
  void visit_concepts(ttx_concept_sink result) const override;
  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override;
  void domain(ttx_abstract self, ttx_domain_result result) const override;
  void callable(ttx_abstract self, ttx_callable_result result) const override;
  void route(ttx_abstract self, ttx_route_result result) const override;
  void finite_extent(ttx_abstract self, ttx_finite_extent_result result)
      const override;

 private:
  ttx_abstract target;
};

}  // namespace Ttx
