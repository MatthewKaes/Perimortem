// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Value {
 public:
  Value() = delete;

  static auto matches(
      const Tetrodotoxin::Language::Definition& definition,
      const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Shader::Language::Program& program,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
