// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/concept/constant.h"

namespace Ttx::Concept {

// Constant proves one complete immutable terminal fact. Once a concept
// resolves to a Constant, that answer is axiomatic for the graph lifetime and
// consumers may retain it without evaluating the concept again.
class Constant : public Abstract {
 public:
  TTX_CONTRACT(Constant, Abstract);

  static auto prove(Abstract& candidate) -> Perimortem::Core::Option<Constant&>;
  static auto prove(const Abstract& candidate)
      -> Perimortem::Core::Option<const Constant&>;

  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Abstract& override;
  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  auto negotiate_interface(const ttx_abstract* requirement) const
      -> ttx_interface override;
};

}  // namespace Ttx::Concept
