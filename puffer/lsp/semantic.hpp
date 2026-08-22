// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Puffer::Lsp {

// Selects the declaration identity represented by one source-associated
// semantic node.
auto semantic_subject(const Ttx::Concept::Abstract& semantic)
    -> const Ttx::Concept::Abstract&;

}  // namespace Puffer::Lsp
