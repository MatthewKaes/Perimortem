// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/app/language/route.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Interpreter {

// Route reads one qualified Type shaped spelling while leaving destination
// category checks to the App policy that consumes it.
class Route {
 public:
  Route() = delete;

  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Route>;
};

}  // namespace Tetrodotoxin::App::Interpreter
