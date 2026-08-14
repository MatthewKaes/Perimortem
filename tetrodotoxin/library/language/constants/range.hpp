// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Range is the exact empty value of one lazy integer Range Type. It carries no
// fabricated element, storage view, or iterator identity.
class Range : public Constant {
 public:
  TTX_CONTRACT(Range, Constant, 0xda0eb0800c554c60, 0x91a04566787dd7de);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Range& type) -> Range& {
    return Expression::create_synthetic<Range>(
        domain, [&](auto source) -> Range { return Range(type, source); });
  }

  constexpr auto get_type() const -> const Types::Range& override {
    return type;
  }

  constexpr auto is_empty() const -> Bool { return True; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.is<Range>() && has_same_type(rhs);
  }

 private:
  constexpr Range(
      const Types::Range& type,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type) {}

  const Types::Range& type;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
