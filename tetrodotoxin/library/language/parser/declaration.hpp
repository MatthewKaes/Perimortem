// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Declaration is the stateless lexical entry point shared by an authored
// Structure and the synthetic Source. Concrete declaration owners parse their
// complete forms, while the receiving Structure routes the exact semantic
// result into its Type, Addressable, or Callable category.
class Declaration {
 public:
  Declaration() = delete;

  // Every Type and Function declaration begins with this exact lexical fact.
  // Field keeps its wider publication policy while reusing the same enclosing
  // declaration transaction.
  static auto parse_visibility(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Visibility>;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      Monograph& source,
      Types::Structure& host) -> Bool;
};

}  // namespace Tetrodotoxin::Library::Language::Parser
