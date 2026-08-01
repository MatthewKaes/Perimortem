// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Fixed accepts one element Type and a nonnegative signed extent. It rejects
// mismatched arguments before the caller Arena constructs the concrete Type.
class Fixed : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Fixed"_view;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(
      Perimortem::Core::View::Vector<Argument> arguments,
      Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override;

  static constexpr Perimortem::Core::Static::Vector<Parameters, 2>
      parameterization = {{Parameters::Type, Parameters::Signed_64}};
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
