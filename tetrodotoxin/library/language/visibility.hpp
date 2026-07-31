// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

namespace Tetrodotoxin::Library::Language {

// Visibility records whether a Library declaration joins its Monograph public
// inventory or remains available only to local lookup.
enum class Visibility : Unsigned_8 {
  Public,
  Private,
};

}  // namespace Tetrodotoxin::Library::Language
