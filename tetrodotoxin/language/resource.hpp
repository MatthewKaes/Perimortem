// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"

namespace Tetrodotoxin::Language {

// Resource exposes bytes retained by a concrete owner. The concrete owner
// keeps both contents and lifetime stable. A consumer may borrow get_value only
// when its domain cannot outlive that dependency domain. A shared domain
// satisfies that contract without another allocation.
class Resource : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(
      Resource,
      Ttx::Concept::Abstract,
      0x84042ad530164a0d,
      0x91069bdadcb11168);

  TTX_NAME("Resource"_view);

  TTX_EMPTY_DOCUMENTATION();

  TTX_CONSTEXPR_INVALID_CONTEXT;

  virtual constexpr auto get_value() const -> Perimortem::Core::View::Bytes = 0;
};

}  // namespace Tetrodotoxin::Language
