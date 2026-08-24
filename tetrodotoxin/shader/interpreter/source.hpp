// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/shader/language/monograph.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Source {
 public:
  Source() = delete;

  static auto parse(
      Language::Monograph& monograph,
      Ttx::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
