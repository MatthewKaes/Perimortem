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
// constructs them.
class View : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "View"_view;

  // Separate parser stages materialize one View family. Sharing this formula
  // identity keeps equal arguments on one Materializations key.
  static auto get_formula() -> const View& {
    static constexpr View formula;
    return formula;
  }

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

  static constexpr Perimortem::Core::Static::Vector<Parameters, 1>
      parameterization = {{Parameters::Type}};
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Provides read-only access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
