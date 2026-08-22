// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Signed is the Constant contract for a signed integer value. Its resolved Type
// remains part of identity while fitting may prove that the value is in range
// for another Signed width.
class Signed : public Constant {
 public:
  auto lower(Llvm::Builder& body) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  TTX_CONTRACT(Signed, Constant);
  using Value = S64;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Signed& type,
      Value value,
      Ttx::Lexical::Anchor anchor) -> Signed& {
    return Expression::create_authored<Signed>(
        domain, anchor,
        [&](auto source) -> Signed { return Signed(type, value, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Signed& type,
      Value value) -> Signed& {
    return Expression::create_synthetic<Signed>(
        domain,
        [&](auto source) -> Signed { return Signed(type, value, source); });
  }

  constexpr auto get_type() const
      -> const Tetrodotoxin::Library::Language::Model::Types::Signed& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Signed>(
        [this, &rhs](const Signed& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    if (!get_type()
             .resolve()
             .is<Tetrodotoxin::Library::Language::Model::Types::Signed>()) {
      return ::False;
    }

    const Ttx::Concept::Abstract& target_type = target.resolve();
    return target_type
        .visit<Tetrodotoxin::Library::Language::Model::Types::Signed>(
            [this](
                const Tetrodotoxin::Library::Language::Model::Types::Signed&
                    selected) {
              Count size = selected.get_size();
              if (size == 0) {
                return ::False;
              }

              if (size >= sizeof(S64)) {
                return ::True;
              }

              S64 limit = S64(1) << (size * 8 - 1);
              return get_value() >= -limit && get_value() < limit ? ::True
                                                                  : ::False;
            },
            [](const Ttx::Concept::Abstract&) { return ::False; });
  }

 private:
  constexpr Signed(
      const Tetrodotoxin::Library::Language::Model::Types::Signed& type,
      Value value,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), value(value) {}

  const Tetrodotoxin::Library::Language::Model::Types::Signed& type;
  Value value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
