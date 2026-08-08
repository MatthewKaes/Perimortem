// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Language {

// Visibility records whether a Library declaration joins its Monograph public
// inventory or remains available only to local lookup.
enum class Visibility : ::Unsigned_8 {
  Public,
  Private,
};

}  // namespace Tetrodotoxin::Library::Language
