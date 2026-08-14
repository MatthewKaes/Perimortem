// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Option is the optional value formula. Its materialized Types retain the
// exact payload identity while this object remains the one immutable rule.
class Option : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Option"_view;

  static auto get_formula() -> const Option& {
    static constexpr Option formula;
    return formula;
  }

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(
      Perimortem::Core::View::Vector<Argument> arguments,
      Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

  static constexpr Perimortem::Core::Static::Vector<Parameters, 1>
      parameterization = {{Parameters::Type}};
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Provides an absent or present value without nullable identity."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
