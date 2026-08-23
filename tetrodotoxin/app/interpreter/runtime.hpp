// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/app/language/runtime.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Interpreter {

// Runtime reads one startup profile and retains only target neutral policy in
// App Language. Package continues to own Resource identity and bytes.
class Runtime {
 public:
  Runtime() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::App::Language::Runtime&>;
};

}  // namespace Tetrodotoxin::App::Interpreter
