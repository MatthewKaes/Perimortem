// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Language {

// Visibility is the authored access mode retained by one Definition. Concrete
// languages decide which modes are legal for the semantic object they create.
enum class Visibility : ::Unsigned_8 {
  Private,
  Public,
  Exposed,
};

}  // namespace Tetrodotoxin::Language
