// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Package::Archive {

// Connects one exported semantic route to a declared logical native artifact
// and its opaque symbol locator. Package proves that the route is unique and
// that the referenced artifact exists. Later semantic and native owners decide
// whether the route and symbol spellings are meaningful.
class Export {
 public:
  constexpr Export(
      Perimortem::Core::View::Bytes semantic_route,
      Perimortem::Core::View::Bytes artifact_id,
      Perimortem::Core::View::Bytes symbol_locator)
      : semantic_route(semantic_route),
        artifact_id(artifact_id),
        symbol_locator(symbol_locator) {}

  constexpr auto get_semantic_route() const -> Perimortem::Core::View::Bytes {
    return semantic_route;
  }

  constexpr auto get_artifact_id() const -> Perimortem::Core::View::Bytes {
    return artifact_id;
  }

  constexpr auto get_symbol_locator() const -> Perimortem::Core::View::Bytes {
    return symbol_locator;
  }

 private:
  Perimortem::Core::View::Bytes semantic_route;
  Perimortem::Core::View::Bytes artifact_id;
  Perimortem::Core::View::Bytes symbol_locator;
};

}  // namespace Tetrodotoxin::Package::Archive
