// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Member selects the concrete parser named by one Definition qualifier. It
// retains no state and returns the real semantic object constructed by that
// owner; the receiving Composite alone decides whether to retain it.
class Member {
 public:
  Member() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Ttx::Concept::Abstract&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser
