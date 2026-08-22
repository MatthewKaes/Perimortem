// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Unsigned is the Constant contract for a nonnegative integer value. The
// resolved Type supplies the authored width while the value remains wide enough
// to prove whether a narrower Unsigned target can represent it.
class Unsigned : public Constant {
 public:
  auto lower(Llvm::Builder& body) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  TTX_CONTRACT(Unsigned, Constant);
  using Value = U64;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type,
      Value value,
      Ttx::Lexical::Anchor anchor) -> Unsigned& {
    return Expression::create_authored<Unsigned>(
        domain, anchor,
        [&](auto source) -> Unsigned { return Unsigned(type, value, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type,
      Value value) -> Unsigned& {
    return Expression::create_synthetic<Unsigned>(
        domain,
        [&](auto source) -> Unsigned { return Unsigned(type, value, source); });
  }

  constexpr auto get_type() const -> const
      Tetrodotoxin::Library::Language::Model::Types::Unsigned& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Unsigned>(
        [this, &rhs](const Unsigned& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    if (!get_type()
             .resolve()
             .is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>()) {
      return ::False;
    }

    const Ttx::Concept::Abstract& target_type = target.resolve();
    return target_type
        .visit<Tetrodotoxin::Library::Language::Model::Types::Unsigned>(
            [this](
                const Tetrodotoxin::Library::Language::Model::Types::Unsigned&
                    selected) {
              Count size = selected.get_size();
              if (size == 0) {
                return ::False;
              }

              if (size >= sizeof(U64)) {
                return ::True;
              }

              return get_value() < (U64(1) << (size * 8)) ? ::True : ::False;
            },
            [](const Ttx::Concept::Abstract&) { return ::False; });
  }

 private:
  constexpr Unsigned(
      const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type,
      Value value,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), value(value) {}

  const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type;
  Value value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
