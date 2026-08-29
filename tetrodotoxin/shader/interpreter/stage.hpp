// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "ttx/bootstrap/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Stage {
 public:
  Stage() = delete;

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Shader::Language::Program& program,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
