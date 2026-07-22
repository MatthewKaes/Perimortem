// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Parser {

// Builtins owns Tetrodotoxin's fixed scalar Type identities and their immutable
// name lookup. It is not a registry: no source, Dialect, or caller can add,
// replace, or remove entries at runtime.
class Builtins {
 public:
  static auto find(Perimortem::Core::View::Bytes name)
      -> Perimortem::Utility::Option<const Ttx::Model::Type&>;
};

}  // namespace Tetrodotoxin::Parser
