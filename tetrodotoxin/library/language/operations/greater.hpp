// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Greater owns one ordered scalar comparison. It retains the selected operand
// Type while its public result remains canonical Bool. Folding leaves those
// authored input and Type identities intact.
class Greater : public Operation {
 public:
  using ClassCatagory = Greater;
  static constexpr Perimortem::System::Uuid contract_id{
    0x37bb18072a5b4e59,
    0x98bc1804a3795115,
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
      Ttx::Lexical::Anchor anchor) -> Greater&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right) -> Greater&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Greater"_view;
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
  Greater(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& left,
      Expression& right,
      Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
