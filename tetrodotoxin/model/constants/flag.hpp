// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/constants/flag.hpp"

namespace Tetrodotoxin::Model::Constants {

// Flag stores one immutable logical value with its real Flag Type edge.
class Flag final : public Ttx::Model::Constants::Flag {
 public:
  constexpr Flag(const Ttx::Model::Types::Flag& type, Bool value)
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
  Ttx::Concept::Reference<Ttx::Model::Types::Flag> type;
  Value value;
};

}  // namespace Tetrodotoxin::Model::Constants
