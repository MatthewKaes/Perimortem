// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Object is the immutable empty value of one generated Object[T] storage Type.
// It carries no allocation while preserving the exact materialized Type.
class Object : public Constant {
 public:
  TTX_CONTRACT(Object, Constant);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type) -> Object& {
    return Expression::create_synthetic<Object>(
        domain, [&](auto anchor) -> Object { return Object(type, anchor); });
  }

  constexpr auto get_type() const -> const Model::Type& override {
    return type;
  }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return has_same_type(rhs) && rhs.is<Object>();
  }

  auto lower(Llvm::Builder& body) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

 private:
  constexpr Object(
      const Model::Type& type,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type) {}

  const Model::Type& type;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
