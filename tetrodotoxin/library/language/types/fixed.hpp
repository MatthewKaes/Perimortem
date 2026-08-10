// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/ranged.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Fixed is one homogeneous range Type. It retains the original signed extent
// and element edge while Ranged exposes the repeated identity without copies.
class Fixed : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(Fixed, Ttx::Model::Type, 0xf1d212690ed5471f, 0x8f47246bd80d2cbe);

  Fixed(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element,
      ::Signed_64 extent)
      : name(name), layout(element, Count(extent)) {
    arguments[0] = Generic::Argument(element);
    arguments[1] = Generic::Argument(extent);
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

  constexpr auto get_layout() const
      -> const Ttx::Model::Layouts::Ranged& override {
    return layout;
  }

  constexpr auto get_arguments() const
      -> Perimortem::Core::View::Vector<Generic::Argument> {
    return arguments;
  }

  constexpr auto get_element_type() const -> const Ttx::Model::Type& {
    return *arguments[0].find<const Ttx::Model::Type&>();
  }

  constexpr auto get_extent() const -> ::Signed_64 {
    return *arguments[1].find<::Signed_64>();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Static::Vector<Generic::Argument, 2> arguments;
  Ttx::Model::Layouts::Ranged layout;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
