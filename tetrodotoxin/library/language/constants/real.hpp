// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Real is an evaluated floating point Constant. Source decimal text may remain
// a Dialect owned literal Expression until a receiving Type selects a format,
// so constructing this contract never silently narrows an exact source literal.
// NaN values compare as one semantic value so Constant equality remains an
// equivalence relation suitable for Generic materialization keys.
class Real : public Constant {
 public:
  TTX_CONTRACT(Real, Constant);
  using Value = Real_64;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Real& type,
      Value value,
      Ttx::Lexical::Anchor anchor) -> Real& {
    return Expression::create_authored<Real>(
        domain, anchor,
        [&](auto source) -> Real { return Real(type, value, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Real& type,
      Value value) -> Real& {
    return Expression::create_synthetic<Real>(
        domain, [&](auto source) -> Real { return Real(type, value, source); });
  }

  constexpr auto get_type() const
      -> const Tetrodotoxin::Library::Language::Model::Types::Real& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Real>(
        [this, &rhs](const Real& selected) {
          if (!has_same_type(rhs)) {
            return ::False;
          }

          Value lhs_value = get_value();
          Value rhs_value = selected.get_value();
          return lhs_value == rhs_value || (__builtin_isnan(lhs_value) &&
                                            __builtin_isnan(rhs_value))
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    const Ttx::Concept::Abstract& source_type = get_type().resolve();
    const Ttx::Concept::Abstract& target_type = target.resolve();
    return source_type
               .is<Tetrodotoxin::Library::Language::Model::Types::Real>() &&
           target_type
               .is<Tetrodotoxin::Library::Language::Model::Types::Real>() &&
           &source_type == &target_type;
  }

 private:
  constexpr Real(
      const Tetrodotoxin::Library::Language::Model::Types::Real& type,
      Value value,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), value(value) {}

  const Tetrodotoxin::Library::Language::Model::Types::Real& type;
  Value value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
