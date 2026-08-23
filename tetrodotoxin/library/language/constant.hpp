// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"

namespace Tetrodotoxin::Library::Language {

// Constant is an immutable Expression already in normal form. It defines no
// parser, operator set, evaluation engine, or lowering representation. Concrete
// value domains expose their payload through derived contracts without
// extending a central tag.
//
// Equality includes resolved Type identity as well as the derived value,
// preserving the distinction between equal bits interpreted by different
// Types.
class Constant : public Expression {
 public:
  TTX_CONTRACT(Constant, Expression);

  // Constant semantic names come from their exact Type. A receiving owner may
  // retain the value through a real Alias or Addressable without renaming this
  // immutable identity.
  TTX_NAME(get_type().get_name());

  // A Constant is a value rather than an authored declaration. When it is
  // stored under a documented name, that prose belongs to the Addressable.
  TTX_EMPTY_DOCUMENTATION();

  virtual constexpr auto get_type() const -> const Model::Type& override = 0;
  virtual constexpr auto equals(const Constant& rhs) const -> Bool = 0;
  virtual auto persist(Archive::Writer&) const -> Bool { return False; }

  auto link_restored(
      const Ttx::Concept::Abstract&,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&>)
      -> Bool override {
    return True;
  }

  constexpr auto operator==(const Constant& rhs) const -> Bool {
    return equals(rhs);
  }
  constexpr auto operator!=(const Constant& rhs) const -> Bool {
    return !equals(rhs);
  }

 protected:
  constexpr explicit Constant(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor) {}

  constexpr auto has_same_type(const Constant& rhs) const -> Bool {
    const Ttx::Concept::Abstract& lhs_type = get_type().resolve();
    const Ttx::Concept::Abstract& rhs_type = rhs.get_type().resolve();
    return lhs_type.is<Model::Type>() && rhs_type.is<Model::Type>() &&
           &lhs_type == &rhs_type;
  }

  static auto have_equal_values(
      const Model::Pack& left,
      const Model::Pack& right) -> Bool;
};

}  // namespace Tetrodotoxin::Library::Language
