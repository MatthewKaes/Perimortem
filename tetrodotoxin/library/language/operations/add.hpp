// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Add owns one ordered scalar sum. It retains the exact left and right
// Expression edges and selects their shared Type during semantic linking.
// Folding projects a sum without changing those authored facts.
class Add : public Operation {
 public:
  using ClassCatagory = Add;
  static constexpr Perimortem::System::Uuid contract_id{
    0x50ca487a954a45d4,
    0xb9881ccf9dd76813,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& left) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right,
      Ttx::Lexical::Anchor anchor) -> Add&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right) -> Add&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Add"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Constant&>, Expression::Error> override;
  auto select_type(Materializations& materializations) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  Add(Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
