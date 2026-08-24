// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Stage {
 public:
  Stage() = delete;

  static auto parse(
      Shader::Language::Program& program,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
