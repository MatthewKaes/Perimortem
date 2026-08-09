// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Expression consumes one complete Library value operand. Postfix Access owns
// its receiver chain before mathematical Operations participate in precedence.
// Each concrete owner parses its complete grammar and constructs its source
// identity. Semantic Types and Addressables connect only when the retained
// owner links the complete Expression graph.
class Expression {
 public:
  Expression() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<Language::Expression&>;

  // Parses one tighter operand with private diagnostics. The caller position
  // advances only when the complete operand succeeds.
  static auto parse_operand(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Ttx::Lexical::Code::Type operation)
      -> Perimortem::Core::Option<Language::Expression&>;

  // Parses one prefix operand with private diagnostics. Postfix operations
  // remain inside the operand while binary operations remain outside it.
  static auto parse_prefix_operand(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser
