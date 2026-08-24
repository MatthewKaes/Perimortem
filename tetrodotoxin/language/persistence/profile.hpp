// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Language::Persistence {

// Profile selects the durable observations retained by each persistent Dialect.
// Complete includes its public and private query contract while Contract keeps
// only the public contract required by dependent consumers. Execution
// remains in the Dialect's compiled Terminal products. Contract stays distinct
// from TTX Interface, which negotiates semantic substitution in a live graph.
enum class Profile : U8 {
  Complete,
  Contract,
};

}  // namespace Tetrodotoxin::Language::Persistence
