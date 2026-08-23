// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/signature.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Declarations {

// Signature interprets the two authored Layouts around one call arrow. The
// retained Signature receives those exact models and owns their later linking
// without retaining this parser or its Cursor.
class Signature {
 public:
  Signature() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host)
      -> Perimortem::Core::Option<Language::Signature&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Declarations
