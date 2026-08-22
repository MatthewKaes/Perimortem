// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Enumeration is one immutable value in an exact authored Enumeration domain.
// The raw storage value remains independent from the optional case Aliases.
class Enumeration : public Constant {
 public:
  auto lower(Llvm::Builder& body) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  TTX_CONTRACT(Enumeration, Constant);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Enumeration& type,
      U64 value) -> Enumeration& {
    return Expression::create_synthetic<Enumeration>(
        domain, [&](auto source) -> Enumeration {
          return Enumeration(type, value, source);
        });
  }

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Enumeration& type,
      U64 value,
      Ttx::Lexical::Anchor anchor) -> Enumeration& {
    return Expression::create_authored<Enumeration>(
        domain, anchor, [&](auto source) -> Enumeration {
          return Enumeration(type, value, source);
        });
  }

  constexpr auto get_type() const -> const Types::Enumeration& override {
    return type;
  }

  constexpr auto get_value() const -> U64 { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Enumeration>(
        [this, &rhs](const Enumeration& selected) {
          return has_same_type(rhs) && value == selected.value ? True : False;
        },
        [](const Ttx::Concept::Abstract&) { return False; });
  }

 private:
  constexpr Enumeration(
      const Types::Enumeration& type,
      U64 value,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), value(value) {}

  const Types::Enumeration& type;
  U64 value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
