// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Model::Parser {

// Pack is the stateless lexical entry point for Library value flow. It owns
// parentheses, separators, source order, explicit slot names, and their shared
// diagnostics. A bare or single value positional form returns the exact child
// Pack. Empty and named forms, along with forms containing several values,
// construct one authored group over those real child identities.
class Pack {
 public:
  Pack() = delete;

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Bool force_parentheses = False)
      -> Perimortem::Core::Option<Language::Model::Pack&>;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Parser
