// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Subtract owns one ordered scalar difference. It retains the exact left and
// right Expression edges and the result Type selected during construction.
// Recursive folding may replace an input but cannot retag the difference.
class Subtract : public Operation {
 public:
  using ClassCatagory = Subtract;
  static constexpr Perimortem::System::Uuid contract_id{
    0x44dabb02f2ff410c,
    0x801b55602cc8e997,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      const Expression& left) -> Perimortem::Utility::Option<const Expression&>;

  Subtract(
      Perimortem::Memory::Allocator::Arena& domain,
      const Expression& left,
      const Expression& right);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Subtract"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) const
      -> Perimortem::Utility::Result<const Expression&, FoldError> override;

 private:
  const Ttx::Concept::Abstract& result_type;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
