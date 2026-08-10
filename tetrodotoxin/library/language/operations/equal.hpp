// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Equal owns exact semantic value equality. Scalar and complete Bytes domains
// keep their own payload contracts while this operation retains only operand
// edges, their selected Type identity, and the canonical Bool result.
class Equal : public Operation {
 public:
  BINARY_OP_CONTRACT(Equal, 0xe0adc31ba73f4e1c, 0xa1d2d2617811c039);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& left) -> Perimortem::Core::Option<Expression&>;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Constant&>, Expression::Error> override;
  auto select_type(Materializations& materializations) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  Equal(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
