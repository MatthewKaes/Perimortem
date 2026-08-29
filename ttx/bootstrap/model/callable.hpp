// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/bootstrap/concept/layout.hpp"
#include "ttx/model/callable.h"

namespace Ttx::Model {

// Callable shares the parameter and result shapes of one invocation. An
// argument Pack fits the parameter Layout and the resulting Pack follows the
// result Layout. Editors, compilers, and language runtimes can all use that
// contract while the defining Dialect keeps the executable body and receiver
// rules that give the Callable its richer meaning.
class Callable : public Concept::Abstract {
 public:
  TTX_CONTRACT(Callable, Abstract);

  static auto prove(Concept::Abstract& candidate)
      -> Perimortem::Core::Option<Callable&>;
  static auto prove(const Concept::Abstract& candidate)
      -> Perimortem::Core::Option<const Callable&>;

  virtual constexpr auto get_parameters() const -> const Concept::Layout& = 0;
  virtual constexpr auto get_results() const -> const Concept::Layout& = 0;

  auto negotiate_interface(const ttx_abstract* requirement) const
      -> ttx_interface override;
};

}  // namespace Ttx::Model
