// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Range is the lazy integer sequence formula. Its materialized Types retain
// the exact element identity while this owner keeps only the immutable rule.
class Range : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Range"_view;

  Range(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& context)
      : Generic(domain, context) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(Perimortem::Core::View::Vector<Argument> arguments) const
      -> Perimortem::Core::Option<const Model::Type&> override;

  static constexpr Perimortem::Core::Static::Vector<Parameters, 1>
      parameterization = {{Parameters::Type}};
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Provides a lazy ascending integer sequence."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
