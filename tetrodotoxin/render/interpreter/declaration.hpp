// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/render/language/monograph.hpp"
#include "ttx/bootstrap/concept/documentation.hpp"

namespace Tetrodotoxin::Render::Interpreter {

class Declaration {
 public:
  Declaration() = delete;

  static auto parse(
      Ttx::Concept::Abstract& host,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Render::Interpreter
