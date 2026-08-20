// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Language::Persistence {

// Profile selects the durable observations each persistent Dialect must retain.
// Complete includes its public and private query contract while Interface
// retains only the public contract required by dependent consumers. Execution
// remains in the Dialect's compiled Terminal products.
enum class Profile : Unsigned_8 {
  Complete,
  Interface,
};

}  // namespace Tetrodotoxin::Language::Persistence
