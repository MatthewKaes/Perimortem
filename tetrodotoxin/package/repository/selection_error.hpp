// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Package::Repository {

// Names the stable caller decision for one rejected Repository selection.
// Repository keeps declaration and selection details in its Info record
// because those facts explain the failure without changing recovery policy.
enum class SelectionError : Unsigned_8 {
  Unknown = Unsigned_8(-1),
  NotDeclared = 0,
  Unreadable,
  InvalidFormat,
  UnsupportedFormat,
  PackageKeyMismatch,
  ArtifactMismatch,
  ArtifactNotDeclared,
};

}  // namespace Tetrodotoxin::Package::Repository
