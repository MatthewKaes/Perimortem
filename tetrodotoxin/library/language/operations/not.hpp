// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Not owns one Flag inverse. It retains the exact operand Expression and its
// resolved Flag Type during semantic linking. Folding projects a value without
// changing that authored input or linked Type.
class Not : public Operation {
 public:
  TTX_CONTRACT(Not, Operation);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand,
      Ttx::Lexical::Anchor anchor) -> Not&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand) -> Not&;

  TTX_NAME("Not"_view);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Constant&>,
          Expression::Error> override;
  auto select_type(const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  Not(Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
