// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/constants/real.hpp"

namespace Tetrodotoxin::Model::Constants {

// Real stores one immutable floating point value with its real Real Type edge.
class Real final : public Ttx::Model::Constants::Real {
 public:
  constexpr Real(const Ttx::Model::Types::Real& type, Real_64 value)
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
  Ttx::Concept::Reference<Ttx::Model::Types::Real> type;
  Value value;
};

}  // namespace Tetrodotoxin::Model::Constants
