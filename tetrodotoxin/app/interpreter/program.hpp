// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/app/language/program.hpp"
#include "ttx/bootstrap/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Interpreter {

// Program reads one authored lifecycle policy and gives the retained route,
// Callable name, and Anchors to the App Language owner.
class Program {
 public:
  Program() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Core::Option<Tetrodotoxin::App::Language::Program&>;
};

}  // namespace Tetrodotoxin::App::Interpreter
