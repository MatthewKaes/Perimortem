// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Or owns left first logical disjunction over two exact canonical Bool
// operands. Its authored edges stay intact while folding decides whether the
// right edge is reachable from the completed left value.
class Or : public Operation {
 public:
  using ClassCatagory = Or;
  static constexpr Perimortem::System::Uuid contract_id{
    0x1079d66d00564cbd,
    0x913b4a5b98aee4f8,
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
      Ttx::Lexical::Anchor anchor) -> Or&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right) -> Or&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Or"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Constant&>, Expression::Error> override;
  auto reaches_next_input(Count folded_input, const Expression& folded) const
      -> Bool override;
  auto select_type(Materializations& materializations) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  Or(Perimortem::Memory::Allocator::Arena& domain,
     Materializations& materializations,
     Expression& left,
     Expression& right,
     Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
