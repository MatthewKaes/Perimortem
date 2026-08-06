// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Not owns one canonical Bool inverse. It retains the exact operand Expression
// while its result keeps the canonical Bool Type selected during construction.
// Recursive folding may replace that input but cannot retag the operation.
class Not : public Operation {
 public:
  using ClassCatagory = Not;
  static constexpr Perimortem::System::Uuid contract_id{
    0x43149308999f46a0,
    0x8530dac9156899bf,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Utility::Option<const Expression&>;

  Not(Perimortem::Memory::Allocator::Arena& domain, const Expression& operand);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Not"_view;
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
