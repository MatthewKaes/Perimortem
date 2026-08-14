// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// And owns left first logical conjunction over two exact canonical Bool
// operands. Its authored edges stay intact while folding decides whether the
// right edge is reachable from the completed left value.
class And : public Operation {
 public:
  BINARY_OP_CONTRACT(And);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Model::Pack& left,
      Ttx::Lexical::Span left_span) -> Perimortem::Core::Option<Expression&>;

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Constant&>,
          Expression::Error> override;
  auto reaches_next_input(Count folded_input, const Expression& folded) const
      -> Bool override;
  auto select_type(Tetrodotoxin::Language::Monograph& source) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  And(Perimortem::Memory::Allocator::Arena& domain,
      Expression& left,
      Expression& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
