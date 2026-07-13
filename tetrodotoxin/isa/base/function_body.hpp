// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/base/definition.hpp"

namespace Tetrodotoxin::Isa::Base {

// FunctionBody is one type-safe connection between a stable TTX function and
// body data owned by the ISA that evaluated it.
class FunctionBody {
 public:
  constexpr FunctionBody() = default;
  constexpr FunctionBody(
      const void* value,
      const void* type,
      Definition definition)
      : value(value), type(type), definition(definition) {}

  constexpr auto get_value() const -> const void* { return value; }
  constexpr auto get_type() const -> const void* { return type; }
  constexpr auto get_definition() const -> const Definition& {
    return definition;
  }

 private:
  const void* value = nullptr;
  const void* type = nullptr;
  Definition definition;
};

}  // namespace Tetrodotoxin::Isa::Base
