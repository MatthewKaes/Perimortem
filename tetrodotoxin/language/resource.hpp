// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Language {

// Resource exposes bytes retained by a concrete owner. The concrete owner
// keeps both contents and lifetime stable. A consumer may borrow get_value only
// when its domain cannot outlive that dependency domain. A shared domain
// satisfies that contract without another allocation.
class Resource : public Ttx::Concept::Abstract {
 public:
  TTX_NAME("Resource"_view);

  virtual constexpr auto get_value() const -> Perimortem::Core::View::Bytes = 0;
};

}  // namespace Tetrodotoxin::Language
