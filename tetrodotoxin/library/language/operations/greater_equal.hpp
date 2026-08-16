// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// GreaterEqual owns one ordered scalar comparison. It retains the selected
// operand Type while its public result remains canonical Bool. Folding leaves
// those authored input and Type identities intact.
class GreaterEqual : public Operation {
 public:
  BINARY_OP_CONTRACT(GreaterEqual);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Model::Pack& left,
      Ttx::Lexical::Span left_span) -> Perimortem::Core::Option<Expression&>;

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Constant&>,
          Expression::Error> override;
  auto select_type(const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  GreaterEqual(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& left,
      Expression& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
