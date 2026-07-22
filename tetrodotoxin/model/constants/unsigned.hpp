// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/constants/unsigned.hpp"

namespace Tetrodotoxin::Model::Constants {

// Unsigned stores one immutable integer value with its real Unsigned Type edge.
class Unsigned final : public Ttx::Model::Constants::Unsigned {
 public:
  constexpr Unsigned(const Ttx::Model::Types::Unsigned& type, Unsigned_64 value)
      : type(type), value(value) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return {};
  }
  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return type.get();
  }
  constexpr auto get_value() const -> Value override { return value; }

 private:
  Ttx::Concept::Reference<Ttx::Model::Types::Unsigned> type;
  Value value;
};

}  // namespace Tetrodotoxin::Model::Constants
