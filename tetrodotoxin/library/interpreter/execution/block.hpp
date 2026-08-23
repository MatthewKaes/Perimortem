// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// Block interprets ordered Statement membership for both braced and compact
// source forms. Every entry retains its real semantic owner while Block keeps
// only source order and lexical lifetime.
class Block {
 public:
  Block() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Language::Model::Callable& function,
      const Language::Model::Type& access_scope,
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
          enclosing_loop = {})
      -> Perimortem::Core::Option<Language::Flow::Block&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
