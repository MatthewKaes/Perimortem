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

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Resource"_view;
  }

  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }

  virtual constexpr auto get_value() const -> Perimortem::Core::View::Bytes = 0;
};

}  // namespace Tetrodotoxin::Language
