// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Constant proves one complete immutable terminal fact. Once a concept
// resolves to a Constant, that answer is axiomatic for the graph lifetime and
// consumers may retain it without evaluating the concept again.
class Constant : public Abstract {
 public:
  TTX_CONTRACT(Constant, Abstract);
};

}  // namespace Ttx::Concept
