// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/base/definition.hpp"

namespace Tetrodotoxin::Isa::Base {

// FunctionBody is one type-safe connection between a stable TTX function and
// body data owned by the ISA that evaluated it. The erased pointer remains an
// implementation detail because it borrows a typed object from source storage.
// A byte view would lose that object identity and invent a size contract that
// the implementation table neither owns nor needs.
class FunctionBody {
 public:
  constexpr FunctionBody() = default;
  explicit constexpr FunctionBody(Definition definition)
      : definition(definition) {}
  template <typename Body>
  constexpr FunctionBody(const Body& value, Definition definition)
      : value(&value), type(&body_type<Body>), definition(definition) {}

  template <typename Body>
  constexpr auto find() const -> const Body* {
    return type == &body_type<Body> ? static_cast<const Body*>(value) : nullptr;
  }

  constexpr auto get_definition() const -> const Definition& {
    return definition;
  }

 private:
  template <typename Body>
  inline static constexpr Unsigned_8 body_type = 0;

  const void* value = nullptr;
  const void* type = nullptr;
  Definition definition;
};

}  // namespace Tetrodotoxin::Isa::Base
