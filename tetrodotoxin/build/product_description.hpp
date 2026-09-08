// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Build {
// One authored request selects an exported Package identity and a Terminal.
struct ProductDescription {
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes package_route;
  Perimortem::Core::View::Bytes terminal_artifact;
  Perimortem::Core::View::Bytes terminal_export;
};
}  // namespace Tetrodotoxin::Build
