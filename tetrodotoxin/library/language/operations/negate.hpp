// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Negate owns one signed or real additive inverse. It retains the exact
// operand Expression and selects its Type during semantic linking. Folding
// projects a value without changing that authored input or linked Type.
class Negate : public Operation {
 public:
  TTX_CONTRACT(Negate, Operation, 0x6f4344a6178e459c, 0x90190b4e89612fbd);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand,
      Ttx::Lexical::Anchor anchor) -> Negate&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand) -> Negate&;

  TTX_NAME("Negate"_view);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Constant&>,
          Expression::Error> override;
  auto select_type(Tetrodotoxin::Language::Monograph& source) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  Negate(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
