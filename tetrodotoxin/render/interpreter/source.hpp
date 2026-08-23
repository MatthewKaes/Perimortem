// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Render::Interpreter {

// Source owns Render body dispatch. The root survives an unsupported
// declaration so editor queries can retain the selected Dialect and source
// evidence while concrete contract owners are added independently.
class Source {
 public:
  Source() = delete;

  static auto parse(Ttx::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Render::Interpreter
