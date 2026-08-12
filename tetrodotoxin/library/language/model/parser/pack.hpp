// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Model::Parser {

// Pack is the stateless lexical entry point for Library value flow. It owns
// parentheses, separators, source order, explicit slot names, and their shared
// diagnostics. A bare or one-value positional form returns the exact child
// Pack; empty, multi-value, and named forms construct one authored group over
// those real child identities.
class Pack {
 public:
  Pack() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Bool force_parentheses = False)
      -> Perimortem::Core::Option<Language::Model::Pack&>;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Parser
