// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// View is the read only contiguous storage formula. Its materialized Types
// retain the element identity while View keeps only the immutable rule that
// constructs them. The owning Monograph supplies the exact byte result from
// Memory so byte concatenation preserves identity across source roots.
class View : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "View"_view;

  View(
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
    "Provides read-only access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
