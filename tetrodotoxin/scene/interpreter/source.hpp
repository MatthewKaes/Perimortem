// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Scene::Interpreter {

// Source composes one Scene source from its real owners. Library receives the
// declarations and executable bodies it understands, while Scene retains only
// its Signals and lifecycle relationships around the same semantic objects.
class Source {
 public:
  Source() = delete;

  static auto parse(
      Language::Monograph& monograph,
      Ttx::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Scene::Interpreter
