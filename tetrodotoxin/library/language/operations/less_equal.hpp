// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// LessEqual owns one ordered scalar comparison. It retains the selected
// operand Type while its public result remains canonical Bool. Folding leaves
// those authored input and Type identities intact.
class LessEqual : public Operation {
 public:
  BINARY_OP_CONTRACT(LessEqual, 0xdf6aad13b2594809, 0xb38cd9d352502973);

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
  auto select_type(Tetrodotoxin::Language::Monograph& source) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  LessEqual(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& left,
      Expression& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
