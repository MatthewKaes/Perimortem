// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

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
  TTX_CONTRACT(Negate, Operation);

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
  auto select_type(const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  Negate(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& operand,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
