// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Equal owns exact semantic value equality. Scalar and complete Bytes domains
// keep their own payload contracts while this operation retains only operand
// edges, their selected Type identity, and the canonical Bool result.
class Equal : public Operation {
 public:
  using ClassCatagory = Equal;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe0adc31ba73f4e1c,
    0xa1d2d2617811c039,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      const Expression& left) -> Perimortem::Utility::Option<const Expression&>;

  Equal(
      Perimortem::Memory::Allocator::Arena& domain,
      const Expression& left,
      const Expression& right);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Equal"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) const
      -> Perimortem::Utility::Result<const Expression&, FoldError> override;

 private:
  const Ttx::Concept::Abstract& operand_type;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
