// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Negate owns one signed or real additive inverse. It retains the exact
// operand Expression and result Type selected during construction. Recursive
// folding may replace that input but cannot retag the operation.
class Negate : public Operation {
 public:
  using ClassCatagory = Negate;
  static constexpr Perimortem::System::Uuid contract_id{
    0x6f4344a6178e459c,
    0x90190b4e89612fbd,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Utility::Option<const Expression&>;

  Negate(
      Perimortem::Memory::Allocator::Arena& domain,
      const Expression& operand);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Negate"_view;
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
