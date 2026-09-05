// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {
class Layout;
}

namespace Tetrodotoxin::Library::Language::Model {

// Iteration is the admission rule owned by a Library sequence form. The loop
// supplies its binding Layout, while the selected Range, Enumeration, or
// contiguous value decides whether that shape can receive one element. Other
// Domains do not acquire iteration policy merely because they produce values.
class Iteration : public Ttx::Concept::Abstract {
 public:
  TTX_NAME("iteration"_view);
  TTX_EMPTY_DOCUMENTATION();

  virtual auto accepts_binding(const Ttx::Concept::Layout& bindings) const
      -> Bool = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Model
