// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "ttx/bootstrap/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Scene::Interpreter {

// Lifecycle recognizes the five Scene entry roles and constructs each one as a
// real Library Function hosted by the Scene instance Object.
class Lifecycle {
 public:
  Lifecycle() = delete;

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Language::Monograph& monograph,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Scene::Interpreter
