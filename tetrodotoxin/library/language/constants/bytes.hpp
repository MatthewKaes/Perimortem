// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/library/language/constant.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Bytes is the Constant contract for immutable byte array data. Quoted source,
// hexadecimal byte literals, and embedded files may all produce this value.
// Library defines no native String constant. A concrete owner may materialize
// its own String Type from these bytes through an ordinary Callable. The graph
// owner keeps the immutable backing storage alive for the Constant.
class Bytes : public Constant {
 public:
  TTX_CONTRACT(Bytes, Constant);
  using Value = Perimortem::Core::View::Bytes;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type,
      Value value,
      Ttx::Lexical::Anchor anchor) -> Bytes& {
    return Expression::create_authored<Bytes>(
        domain, anchor,
        [&](auto source) -> Bytes { return Bytes(type, value, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type,
      Value value) -> Bytes& {
    return Expression::create_synthetic<Bytes>(
        domain,
        [&](auto source) -> Bytes { return Bytes(type, value, source); });
  }

  constexpr auto get_type() const -> const Model::Type& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Bytes>(
        [this, &rhs](const Bytes& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

 private:
  constexpr Bytes(
      const Model::Type& type,
      Value value,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), value(value) {}

  const Model::Type& type;
  Value value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
