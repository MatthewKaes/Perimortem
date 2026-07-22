// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Parser {

// Builtins owns Tetrodotoxin's fixed scalar Type identities and their immutable
// name lookup. Generic formulas belong to the active Environment and therefore
// resolve through the ordinary source context.
class Builtins {
 public:
  static auto find(Perimortem::Core::View::Bytes name)
      -> Perimortem::Utility::Option<const Ttx::Model::Type&>;
};

}  // namespace Tetrodotoxin::Parser
