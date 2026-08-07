// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Multiply owns one binary scalar product. It retains the exact left and right
// Expression edges and selects their shared Type during semantic linking.
// Folding projects a product while every authored input and linked Type remain.
class Multiply : public Operation {
 public:
  using ClassCatagory = Multiply;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5c072da344144a09,
    0x8b5fc8dd12d91e3a,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& left) -> Perimortem::Utility::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right,
      Ttx::Lexical::Anchor anchor) -> Multiply&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right) -> Multiply&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Multiply"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations)
      -> Perimortem::Utility::Result<
          Perimortem::Utility::Option<Constant&>,
          Expression::Error> override;
  auto select_type(Materializations& materializations) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override;

 private:
  Multiply(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right,
      Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
