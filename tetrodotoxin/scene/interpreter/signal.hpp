// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Scene::Interpreter {

// Signal reads the compact event declaration owned by Scene and retains its
// optional payload as a delayed Library Type edge.
class Signal {
 public:
  Signal() = delete;

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Language::Monograph& monograph,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Scene::Interpreter
